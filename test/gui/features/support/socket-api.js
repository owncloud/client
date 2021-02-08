#!/usr/bin/env babel-node

import { Socket } from 'net';
import rsvp from 'rsvp';
import { exec, spawn } from 'child-process-promise';
import { createReadStream } from 'fs';
import tmp from 'tmp';
import { execSync } from 'child_process';

import {
    argv,
    exit,
 } from 'process';

// import sleep from 'sleep';
// sleep requires a native module and therefore a compiler to install which we would like to avoid
let sleep = {
    msleep: function(msecs) {
        execSync('sleep ' + msecs/1000);
    }
};

let usleep = function(msecs) {
    return new Promise(resolve => setTimeout(resolve, msecs));
}

export class SocketApi {
    constructor({world}) {
        this.world = world;

        this.counter = 0;
        this.promises = {};
        this.screenshotNo = 0;

        this.clientEnv = world.clientEnv;
        this.folderStatusPromise = '';
        this.shareStatusPromise = '';
        this.itemName = '';
    }

    async setup(configSetupArg = '', socketPath) {
        let defaultSpawnArg
        defaultSpawnArg = ['--logfile', '/tmp/client.log']

        if(configSetupArg){
            defaultSpawnArg = defaultSpawnArg.concat(configSetupArg)
        }
        // console.log('spawn_client', this.world.parameters.spawn_client);
        if (this.world.parameters.spawn_client !== false) {
            // TODO: do not run owncloud from path
            this.ownCloudProcess = spawn('owncloud', defaultSpawnArg, {env: this.clientEnv}).then(null, (err) => {
                // console.log('Spawn error:', err);
            }).childProcess;
        }

        return this._connect().then(() => {
            // console.log('SocketApi: connected');
        });
    }

    teardown() {
        // console.log('teardown', this.socket);
        if(this.socket) {
            this.socket.destroy();
        }

        if(this.ownCloudProcess && this.world.parameters.kill_client !== false) {
            this.ownCloudProcess.kill('SIGKILL');
            sleep.msleep(500);
        }
    }

    _connectImpl() {
        // console.log(this.clientEnv.XDG_RUNTIME_DIR + '/ownCloud/socket');
        this.socket.connect(this.clientEnv.XDG_RUNTIME_DIR + '/ownCloud/socket');

    }

    _connect() {
        return new Promise((resolve, reject) => {
            this.socket = Socket();

            this.socket.on('data', (d) => {
                var data = d.toString();
                data.split('\n').forEach((line) => {
                    if (line.startsWith('RESOLVE') || line.startsWith('REJECT')) {
                        this._dataLineHandler(line);
                    } else if (line.startsWith('STATUS:OK') && line.endsWith(this.itemName)) {
                        this.folderStatusPromise.resolve(line)
                    } else if (line.startsWith('SHARE:') && line.endsWith(this.itemName)) {
                        if (line.startsWith('SHARE:OK')) {
                            this.shareStatusPromise.resolve(line)
                        } else {
                            this.shareStatusPromise.reject(line)
                        }
                    } else {
                        // console.log(line)
                    }
                })
            });

            this.socket.on('connect', resolve);
            this.socket.on('error', (...args) => {
                // console.log('connect error');
                sleep.msleep(50);
                this._connectImpl();
            });
            this._connectImpl();
        });
    }

    _dataLineHandler(line) {
        var split = line.split('|');
        if (split[0] == 'RESOLVE') {
            this.promises[split[1]].resolve(split[2].trim());
        } else if (split[0] == 'REJECT') {
            this.promises[split[1]].reject(split[2].trim());
        }
    }

    //  helper to send a \n terminated message to `this.socket`
    send(message) {
        this.socket.write(message + '\n');
    }

    invokeCommand(command, args) {
        var promise = new rsvp.Promise((resolve, reject) => {
            this.promises[this.counter] = { resolve: resolve, reject: reject };
        });

        this.send(command+':'+this.counter+'|'+JSON.stringify(args))

        this.counter++;

        return promise;
    }

    checkFolderStatus(command, folderName) {
        this.itemName = folderName
        var promise = new rsvp.Promise((resolve, reject) => {
            this.folderStatusPromise = { resolve: resolve, reject: reject };
        });

        this.send(command + folderName)

        return promise;
    }

    checkShareStatus(command, folderName) {
        this.itemName = folderName
        var promise = new rsvp.Promise((resolve, reject) => {
            this.shareStatusPromise = { resolve: resolve, reject: reject };
        });

        this.send(command + folderName)
        return promise;
    }

    getValue(objectName, propertyName) {
        return this.invokeCommand('ASYNC_GET_WIDGET_PROPERTY', {objectName: objectName, property: propertyName});
    }

    setValue(objectName, propertyName, value) {
        return this.invokeCommand('ASYNC_SET_WIDGET_PROPERTY', {objectName: objectName, property: propertyName, value: value}).then(this.takeScreenshot);
    }

    invokeMethod(objectName, method) {
        return this.invokeCommand('ASYNC_INVOKE_WIDGET_METHOD', {objectName: objectName, method: method}).then(this.takeScreenshot);
    }

    waitForSignal(objectName, signalSignature) {
        return this.invokeCommand('ASYNC_WAIT_FOR_WIDGET_SIGNAL', {objectName: objectName, signalSignature: signalSignature});
    }

    listObjects() {
        return this.invokeCommand('ASYNC_LIST_WIDGETS', {});
    }

    triggerMenuAction(objectName, actionName) {
        return this.invokeCommand('ASYNC_TRIGGER_MENU_ACTION', {objectName: objectName, actionName: actionName}).then(this.takeScreenshot);
    }

    click(queryString) {
        return this.invokeMethod(queryString, 'click');
    }
    clickAndWaitForSignal(queryString, signalQueryString, signalSignature) {
        let signalPromise = this.waitForSignal(signalQueryString, signalSignature);
        return this.click(queryString).then(() => {
            return signalPromise;
        });
    }

    takeScreenshot = async (value) => {
        let file = tmp.fileSync();
        let fileName = file.name;

        var screenshot   = 'import -window root -delay 0 PNG:' + fileName;

        await exec(screenshot);

        await this.world.attach(createReadStream(fileName), 'image/png');

        file.removeCallback();

        return value;
    }
}

if (require.main === module) {
    const socketApi = new SocketApi({ world: {
        parameters: {
            spawn_client: false,
        },
        attach: () => {},
        clientEnv: Object.assign({}, process.env, {}),
    }});

    const command = argv[2];
    const args = argv.slice(3);
    console.log(command, args);

    socketApi.setup().then(() => {
        return socketApi[command](...args).then((result) => {
            socketApi.teardown();

            console.log(result);
        })
    }).catch((err) => {
        socketApi.teardown();

        console.error('Error returned from SocketApi: ', err);
        exit(1);
    });
}
