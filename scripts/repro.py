"""Does a Qt QNAM GET to http://localhost:<port> reach a QTcpServer on QHostAddress::LocalHost?"""
import sys, time
from PySide6.QtCore import QCoreApplication, QTimer, QUrl, QLibraryInfo, QByteArray
from PySide6.QtNetwork import (QTcpServer, QHostAddress, QNetworkAccessManager, QNetworkRequest,
                               QNetworkProxyFactory, QNetworkProxyQuery, QHostInfo)

app = QCoreApplication(sys.argv)
print("Qt", QLibraryInfo.version().toString(), "| system proxy config:", QNetworkProxyFactory.usesSystemConfiguration())
srv = QTcpServer()
assert srv.listen(QHostAddress.LocalHost, 0)
port = srv.serverPort()
print("listening on", srv.serverAddress().toString(), port)

def on_conn():
    s = srv.nextPendingConnection()
    print("  server: connection from", s.peerAddress().toString())
    s.readyRead.connect(lambda: (s.write(b"HTTP/1.1 302 Found\r\nLocation: owncloud://success\r\n"
                                         b"Content-Length: 0\r\nConnection: close\r\n\r\n"), s.disconnectFromHost()))
srv.newConnection.connect(on_conn)

t = time.monotonic(); hi = QHostInfo.fromName("localhost")
print("QHostInfo(localhost):", [a.toString() for a in hi.addresses()], f"{time.monotonic() - t:.2f}s")
for h in ("127.0.0.1", "localhost"):
    t = time.monotonic()
    px = QNetworkProxyFactory.systemProxyForQuery(QNetworkProxyQuery(QUrl(f"http://{h}:{port}/")))
    print(f"systemProxyForQuery({h}):", [p.type().name for p in px], f"{time.monotonic() - t:.2f}s")

nam = QNetworkAccessManager()
queue = ["localhost", "127.0.0.1", "localhost"]
def nxt():
    if not queue:
        app.quit(); return
    h = queue.pop(0)
    req = QNetworkRequest(QUrl(f"http://{h}:{port}/?code=x"))
    req.setAttribute(QNetworkRequest.RedirectPolicyAttribute, QNetworkRequest.ManualRedirectPolicy)
    req.setTransferTimeout(30000)
    t = time.monotonic(); rep = nam.get(req)
    def done():
        try:
            loc = rep.rawHeader("Location").data()
            status = rep.attribute(QNetworkRequest.HttpStatusCodeAttribute)
            print(f"GET {h}: error={rep.error().name} ({rep.errorString()}) status={status} location={loc!r} {time.monotonic() - t:.2f}s", flush=True)
        finally:
            nxt()
    rep.finished.connect(done)
QTimer.singleShot(0, nxt)
QTimer.singleShot(90000, lambda: (print("GLOBAL TIMEOUT, pending:", queue), app.quit()))
app.exec()
