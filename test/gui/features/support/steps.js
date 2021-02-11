import { Given, When, Then, After, defineParameterType } from 'cucumber';
import { exec } from 'child-process-promise';
import { pathExists, ensureDir } from 'fs-extra';
import request from 'request-promise-native';
import { promisify } from 'util';
import owncloud from 'js-owncloud-client';
import fs from 'fs';
import {
    join,
    dirname,
} from 'path';

import { env } from 'process';


import promiseSequence from './rsvp-party/sequence';
const codify = require('./helpers/codify')
const assert = require('assert')

const usleep = promisify(setTimeout);
const writeFile = promisify(fs.writeFile);
const createParentDirectory = function(filename) {
    return ensureDir(dirname(filename));
};

const confdir = '/tmp/owncloud-gui-test-home/'
const confFilePath = confdir + 'owncloud.cfg'

const login = async (url, username, password) => {
    console.log('creds: ', url, username, password);
    // await exec('curl http://build-linux-webserver-owncloud-10-1-1/index.php/login');
    const oc = new owncloud(url);
    await oc.login(username, password);
    return oc;
};

const defaultLogin = () => {
    // console.log(env);
    return login(env.OWNCLOUD_URL, 'admin','admin');
};

const handleEnvVars = function(value) {
    var regexp = new RegExp(/\${ENV:(.*?)}/gm);
    let vars = Array.from(value.matchAll(regexp)).map(envVar => {
        return envVar[1];
    });

    // _.unique(vars);

    vars.forEach(envVar => {
        value = value.replace("${ENV:"+envVar+"}", env[envVar]);
    });

    return value;
};

Given('the user has started the client', function () {
    return this.socketApi.setup();
});

Given('a client is run for the first time', function () {
    return this.socketApi.setup();
});

When('the user adds the first account with', async function (dataTable) {
    codify.replaceInlineTable(dataTable)
    const settings = dataTable.rowsHash()
    await this.socketApi.setValue('#leUrl', 'text', settings.server);
    await this.socketApi.invokeMethod('#__qt__passive_wizardbutton1', 'click');
    await this.socketApi.setValue('#leUsername', 'text', settings.user);
    await this.socketApi.setValue('#lePassword', 'text', settings.password);
    await this.socketApi.invokeMethod('#__qt__passive_wizardbutton1', 'click');
    await this.socketApi.invokeMethod('#__qt__passive_wizardbutton1', 'click');
});

When('the user adds an account with', async function (dataTable) {
    codify.replaceInlineTable(dataTable)
    const settings = dataTable.rowsHash()

    await this.socketApi.getValue('#settingsdialog_toolbutton_Add account', 'visible')
    await this.socketApi.invokeMethod('#settingsdialog_toolbutton_Add account', 'click')
    await this.socketApi.invokeMethod('#leUrl', 'click')
    await this.socketApi.setValue('#leUrl', 'text', settings.server);
    await this.socketApi.invokeMethod('#__qt__passive_wizardbutton1', 'click');
    await this.socketApi.invokeMethod('#leUsername', 'click');
    await this.socketApi.setValue('#leUsername', 'text', settings.user);
    await this.socketApi.invokeMethod('#lePassword', 'click');
    await this.socketApi.setValue('#lePassword', 'text', settings.password);
    await this.socketApi.invokeMethod('#__qt__passive_wizardbutton1', 'click');
    await this.socketApi.invokeMethod('#__qt__passive_wizardbutton1', 'click');
});

Then('an account should be displayed with the displayname {string} and host {code}', async function (displayname, host) {
    let value = await this.socketApi.getValue(
        `#settingsdialog_toolbutton_${displayname}@${host}`, 'text'
    );
    assert.equal(value, displayname)
});


When(/^I enter (.*) for (.*) on (.*)$/, function (value, propertyName, objectName) {
    value = handleEnvVars(value);
    return this.socketApi.setValue(objectName, propertyName, value);
});

When(/^I enter (.*) into (.*)$/, {timeout: 30*1000 }, function (value, objectName) {
    value = handleEnvVars(value);
    console.log('*********** ENTER', value);
    return this.socketApi.setValue(objectName, 'text', value);
});

When(/^I click ([^\s]+)$/, function (objectName) {
    objectName = handleEnvVars(objectName);
    console.log('objectName', objectName);
    return this.socketApi.invokeMethod(objectName, 'click');
});

When(/^I click (.*) and wait for (.*) from (.*)$/, function (objectName, signalSignature, signalObjectName) {
    // let signalPromise = this.socketApi.waitForSignal(signalObjectName, signalSignature);
    // return this.socketApi.invokeMethod(objectName, 'click').then(() => {
    //     return signalPromise;
    // });
    return this.socketApi.clickAndWaitForSignal(objectName, signalObjectName, signalSignature);
});

When(/^I wait for signal (.*) from (.*)$/, function () {
    return this.socketApi.waitForSignal(objectName, signalSignature);
});

When(/^(.*) is visible$/, async function (queryString) {
    queryString = handleEnvVars(queryString);

    // console.log({queryString});

    return promiseUntil(async () => {
        return this.socketApi.getValue(queryString, 'visible').then(visible => {
            if(!visible) {
                console.log("not visible");
                throw new Exception('not visible');
            }
        });
    });
});


When('I wait {int} seconds', {timeout: 60 * 1000}, async function (secs) {
    await usleep(secs*1000);
    return this.socketApi.takeScreenshot();
});

Then(/^Settings dialog shows settings for (.*)$/, async function (url) {
    url = handleEnvVars(url);
    let urlValue = await this.socketApi.getValue('#Settings', 'currentPage.accountState.account.url');
    assert.equal(url, urlValue);
});

// When('I log in at https:\/\/demo.owncloud.com as user demo with password demo', async function () {
When('I log in at {string} as user {string} with password {string}', async function (url, user, password) {
    let nextButtonId = '#__qt__passive_wizardbutton1';

    await this.socketApi.setValue('#leUrl', 'text', url);
    await this.socketApi.clickAndWaitForSignal(nextButtonId, '#owncloudWizard', 'currentIdChanged(int)');

    await this.socketApi.setValue('#leUsername', 'text', user);
    await this.socketApi.setValue('#lePassword', 'text', password);
    await this.socketApi.clickAndWaitForSignal(nextButtonId, '#owncloudWizard', 'currentIdChanged(int)');
    await this.socketApi.clickAndWaitForSignal(nextButtonId, '#owncloudWizard', 'finished(int)'); // "Connect..."
});

Given('folder {string} exists', async function (string) {
    let cmd = 'mkdir -p ' + process.env.CLIENT_SYNC_PATH + '/' + string;
    return exec(cmd);
});

Then('folder {string} should exist', async function (string) {
    let path = process.env.CLIENT_SYNC_PATH + '/' + string;
    return pathExists(path).then((exists) => {
        if (!exists) {
            throw new Error("Folder " + path + " does not exist.");
        }
    });
});

Given('list objects', async function () {
    return this.socketApi.listObjects().then(console.log);
});

Then('{string} has icon {string} set to {string}', async function (queryString, propertyPath, iconName) {
    return this.socketApi.invokeCommand('ASYNC_ASSERT_ICON_IS_EQUAL', {queryString, propertyPath, iconName});
});

Then('{string} has {string} set to {string}', async function (queryString, propertyPath, checkValue) {
    return this.socketApi.getValue(queryString, propertyPath).then((value) => {
        if(value !== checkValue) {
            throw new Error(propertyPath + ' on ' + queryString + ' is set to ' + value);
        }
    });
});

When('a file {string} is created locally with content {string}', async function (filename, content) {
    const final_filename = join(process.env.CLIENT_SYNC_PATH, filename);

    await createParentDirectory(final_filename);
    return writeFile(final_filename, content);
});

When('a directory {string} is created locally', async function (directory) {
    const final_path = join(process.env.CLIENT_SYNC_PATH, directory);

    return ensureDir(final_path);
});


const promiseUntil = async function(promiseFunc) {
    try {
        await usleep(250);

        // await is needed to catch rejections
        return await promiseFunc();
    } catch(e) {
        // console.log('promise failed', e);
        return promiseUntil(promiseFunc);
    }
};

Then('a file {string} becomes available on the server with content {string}', {timeout: 60*1000}, async function (filename, content) {
    const oc = await defaultLogin();

    const actualContent = await promiseUntil(async () => {
        return oc.files.getFileContents(filename);
    });

    assert.equal(actualContent, content);
});

Then('a directory {string} becomes available on the server', {timeout: 60 * 1000}, async function (directory) {
    const oc = await defaultLogin();

    const fileInfo = await promiseUntil(async () => {
        return oc.files.fileInfo(directory);
    });

    assert.equal(fileInfo.type,  'dir');
});

Given('server root is empty', {timeout: 15 * 1000}, async function () {
    const oc = await defaultLogin();

    const files = await oc.files.list('/');
    files.shift();

    return promiseSequence(files.map((file) => {
        return () => {
            return oc.files.delete(file.name);
        };
    }))
});

function setupUser(userId, context){
    context = codify.replaceInlineCode(context)
    fs.writeFileSync(confFilePath, context)
    // console.log('completed writing file')
}

Given('user {string} has set up a client with these settings:', function(userId, context) {
    const configSetupArg = ['--confdir', confdir]
    setupUser(userId, context)
    return this.socketApi.setup(configSetupArg);

})

async function isShareDialogueVisible(socket, itemName) {
    return await new Promise(resolve => {
        const interval = setInterval(() => {
            socket.checkShareStatus('SHARE:', itemName).then(() => {
                clearInterval(interval);
                resolve();
            })
        }, 4000);
    });
}

function isItemSynced(type, itemName, socket){
    if(type !== 'FILE' && type !== 'FOLDER'){
        throw new Error("type must be 'FILE' or 'FOLDER'");
    }
    return socket.checkFolderStatus('RETRIEVE_' + type + '_STATUS:', itemName).then(r =>{
        console.log(r)
    })
}

function isFolderSynced(socket, folderName){
    return isItemSynced('FOLDER', folderName, socket)
}

function isFileSynced(socket, fileName){
    return isItemSynced('FILE', fileName, socket)
}

When('the user adds {string} as collaborator of resource {string} with permissions {string} using the client-UI', async function (shareWithUser, resource, permissions) {
    resource = (codify.replaceInlineCode(resource)).replace('//', '/')
    await isFileSynced(this.socketApi, resource)
    await isShareDialogueVisible(this.socketApi, resource)

    //wait for share dialogue box to open
    await usleep(2000)

    await this.socketApi.getValue('#shareeLineEdit', 'visible')
    await this.socketApi.invokeMethod('#shareeLineEdit', 'click')
    await this.socketApi.setValue('#shareeLineEdit', 'text', shareWithUser);
});

After(function (testCase) {
    // console.log('teardown', this.socketApi);
    this.socketApi.teardown();
});
