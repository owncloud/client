"""Does a Qt QNAM GET to http://localhost:<port> reach a QTcpServer on QHostAddress::LocalHost?"""
import sys, time
from PySide6.QtCore import QCoreApplication, QTimer, QUrl, QLibraryInfo
from PySide6.QtNetwork import (QTcpServer, QTcpSocket, QHostAddress, QNetworkAccessManager, QNetworkRequest,
                               QHostInfo, QAbstractSocket)

app = QCoreApplication(sys.argv)
print("Qt", QLibraryInfo.version().toString(), flush=True)
srv = QTcpServer()
assert srv.listen(QHostAddress.LocalHost, 0)
port = srv.serverPort()

def on_conn():
    s = srv.nextPendingConnection()
    print("  server: connection from", s.peerAddress().toString(), flush=True)
    s.readyRead.connect(lambda: (s.write(b"HTTP/1.1 302 Found\r\nLocation: owncloud://success\r\n"
                                         b"Content-Length: 0\r\nConnection: close\r\n\r\n"), s.disconnectFromHost()))
srv.newConnection.connect(on_conn)
print("QHostInfo(localhost):", [a.toString() for a in QHostInfo.fromName("localhost").addresses()], flush=True)

# QTcpSocket, direct addresses and by name
for target in ("::1", "127.0.0.1", "localhost"):
    sock = QTcpSocket(); t = time.monotonic()
    sock.connectToHost(target, port)
    ok = sock.waitForConnected(10000)
    print(f"QTcpSocket {target}: connected={ok} error={sock.error().name} peer={sock.peerAddress().toString()} {time.monotonic() - t:.2f}s", flush=True)
    sock.abort()

nam = QNetworkAccessManager()
queue = ["[::1]", "127.0.0.1", "localhost"]
def nxt():
    if not queue:
        app.quit(); return
    h = queue.pop(0)
    req = QNetworkRequest(QUrl(f"http://{h}:{port}/?code=x"))
    req.setAttribute(QNetworkRequest.RedirectPolicyAttribute, QNetworkRequest.ManualRedirectPolicy)
    req.setTransferTimeout(20000)
    t = time.monotonic(); rep = nam.get(req)
    def done():
        try:
            status = rep.attribute(QNetworkRequest.HttpStatusCodeAttribute)
            print(f"QNAM GET {h}: error={rep.error().name} status={status} {time.monotonic() - t:.2f}s", flush=True)
        finally:
            nxt()
    rep.finished.connect(done)
QTimer.singleShot(0, nxt)
QTimer.singleShot(90000, lambda: (print("GLOBAL TIMEOUT, pending:", queue), app.quit()))
app.exec()
