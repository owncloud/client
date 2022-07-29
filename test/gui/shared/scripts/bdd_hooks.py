# -*- coding: utf-8 -*-

# This file contains hook functions to run as the .feature file is executed.
#
# A common use-case is to use the OnScenarioStart/OnScenarioEnd hooks to
# start and stop an AUT, e.g.
#
# @OnScenarioStart
# def hook(context):
#     startApplication("addressbook")
#
# @OnScenarioEnd
# def hook(context):
#     currentApplicationContext().detach()
#
# See the section 'Performing Actions During Test Execution Via Hooks' in the Squish
# manual for a complete reference of the available API.
import shutil
from tempfile import gettempdir
import urllib.request
import os
import builtins
from helpers.StacktraceHelper import getCoredumps, generateStacktrace
from datetime import datetime

import subprocess
import signal
import re
import glob

# this will reset in every test suite
previousFailResultCount = 0
previousErrorResultCount = 0

# video recording process
video = None

def sanitizeFilename(name):
    return name.replace(" ", "_").replace("/", "_").strip(".")

def createVideoFilename(title):
    if os.environ["GUI_TEST_REPORT_DIR"]:
        video_dir = os.environ["GUI_TEST_REPORT_DIR"] + '/videos'
    else:
        video_dir = "../videos"
    if not os.path.exists(video_dir):
        os.makedirs(video_dir)

    return os.path.join(video_dir, sanitizeFilename(title) + ".mp4")

def recordScreen(feature):
    global video

    video_file = createVideoFilename(feature)

    video_size = subprocess.run("xprop -notype -len 16 -root _NET_DESKTOP_GEOMETRY".split(" "), check=True, capture_output=True)
    video_size = subprocess.run(['cut', '-c', '25-'], input=video_size.stdout, capture_output=True)
    video_size = str(video_size.stdout)
    match = re.search('[\d, ]+', video_size)
    video_size = video_size[match.start():match.end()]
    video_size = video_size.replace(', ', 'x')

    ffmpeg = os.getenv("FFMPEG_PATH", "ffmpeg")
    cmd = "%s -video_size %s -framerate 25 -f x11grab -i :0.0 %s" % (ffmpeg, video_size, video_file)
    print(cmd)
    video = subprocess.Popen(cmd.split(" "), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, start_new_session=True)
    print("Recording screen...")

@OnScenarioStart
def hook(context):
    if os.getenv('RECORD_VIDEO'):
        recordScreen(context.title)

    from configparser import ConfigParser

    CONFIG_ENV_MAP = {
        'localBackendUrl': 'BACKEND_HOST',
        'secureLocalBackendUrl': 'SECURE_BACKEND_HOST',
        'maxSyncTimeout': 'MAX_SYNC_TIMEOUT',
        'minSyncTimeout': 'MIN_SYNC_TIMEOUT',
        'lowestSyncTimeout': 'LOWEST_SYNC_TIMEOUT',
        'middlewareUrl': 'MIDDLEWARE_URL',
        'clientConfigFile': 'CLIENT_LOG_FILE',
        'clientRootSyncPath': 'CLIENT_ROOT_SYNC_PATH',
        'tempFolderPath': 'TEMP_FOLDER_PATH',
    }

    DEFAULT_CONFIG = {
        'localBackendUrl': 'https://localhost:9200/',
        'secureLocalBackendUrl': 'https://localhost:9200/',
        'maxSyncTimeout': 60,
        'minSyncTimeout': 5,
        'lowestSyncTimeout': 1,
        'middlewareUrl': 'http://localhost:3000/',
        'clientConfigFile': '-',
        'clientRootSyncPath': '/tmp/client-bdd/',
        'tempFolderPath': gettempdir(),
    }

    # log tests scenario title on serverlog file
    if os.getenv('CI'):
        guiTestReportDir = os.environ.get("GUI_TEST_REPORT_DIR")
        f = open(guiTestReportDir + "/serverlog.log", "a")
        f.write(
            str((datetime.now()).strftime("%H:%M:%S:%f"))
            + "\tBDD Scenario: "
            + context._data["title"]
            + "\n"
        )
        f.close()

    # read configs from environment variables
    context.userData = {}
    for key, value in CONFIG_ENV_MAP.items():
        context.userData[key] = os.environ.get(value, '')

    # try reading configs from config.ini
    cfg = ConfigParser()
    try:
        cfg.read('../config.ini')
        for key, value in context.userData.items():
            if value == '':
                context.userData[key] = cfg.get('DEFAULT', CONFIG_ENV_MAP[key])
    except Exception as err:
        test.log(str(err))

    # Set the default values if empty
    for key, value in context.userData.items():
        if value == '':
            context.userData[key] = DEFAULT_CONFIG[key]
        elif key == 'maxSyncTimeout' or key == 'minSyncTimeout':
            context.userData[key] = builtins.int(value)
        elif key == 'clientRootSyncPath' or 'tempFolderPath':
            # make sure there is always one trailing slash
            context.userData[key] = value.rstrip('/') + '/'

    # initially set user sync path to root
    # this path will be changed according to the user added to the client
    # e.g.: /tmp/client-bdd/Alice
    context.userData['currentUserSyncPath'] = context.userData['clientRootSyncPath']

    if not os.path.exists(context.userData['clientRootSyncPath']):
        os.makedirs(context.userData['clientRootSyncPath'])

    if not os.path.exists(context.userData['tempFolderPath']):
        os.makedirs(context.userData['tempFolderPath'])

    req = urllib.request.Request(
        os.path.join(context.userData['middlewareUrl'], 'init'),
        headers={"Content-Type": "application/json"},
        method='POST',
    )
    try:
        urllib.request.urlopen(req)
    except urllib.error.HTTPError as e:
        raise Exception(
            "Step execution through test middleware failed. Error: " + e.read().decode()
        )

def isTestFailed():
    global previousFailResultCount, previousErrorResultCount
    return (previousFailResultCount > 0 or previousErrorResultCount > 0)

@OnScenarioEnd
def hook(context):
    if os.getenv('RECORD_VIDEO'):
        global video
        os.killpg(os.getpgid(video.pid), signal.SIGTERM)
        print("Video saved!")
        print(glob.glob("/drone/src/test/guiReportUpload/videos/*"))

    # Currently, this workaround is needed because we cannot find out a way to determine the pass/fail status of currently running test scenario.
    # And, resultCount("errors")  and resultCount("fails") return the total number of error/failed test scenarios of a test suite.
    global previousFailResultCount, previousErrorResultCount

    # capture a screenshot if there is error or test failure in the current scenario execution
    if isTestFailed() and os.getenv('CI'):
        import gi

        gi.require_version('Gtk', '3.0')
        from gi.repository import Gdk

        window = Gdk.get_default_root_window()
        pb = Gdk.pixbuf_get_from_window(window, *window.get_geometry())

        # scenario name can have "/" which is invalid filename
        filename = (
            context._data["title"].replace(" ", "_").replace("/", "_").strip(".")
            + ".png"
        )
        directory = os.environ["GUI_TEST_REPORT_DIR"] + "/screenshots"

        if not os.path.exists(directory):
            os.makedirs(directory)

        pb.savev(os.path.join(directory, filename), "png", [], [])
    elif not isTestFailed() and os.getenv('RECORD_VIDEO'):
        video_file = createVideoFilename(context.title)
        try:
            os.unlink(video_file)
        except:
            pass

    # Detach (i.e. potentially terminate) all AUTs at the end of a scenario
    for ctx in applicationContextList():
        ctx.detach()
        # ToDo wait smarter till the app died
        snooze(context.userData['minSyncTimeout'])

    # delete local files/folders
    for filename in os.listdir(context.userData['clientRootSyncPath']):
        test.log("Deleting: " + filename)
        file_path = os.path.join(context.userData['clientRootSyncPath'], filename)
        try:
            if os.path.isfile(file_path) or os.path.islink(file_path):
                os.unlink(file_path)
            elif os.path.isdir(file_path):
                shutil.rmtree(file_path)
        except Exception as e:
            test.log('Failed to delete' + file_path + ". Reason: " + e + '.')

    # search coredumps after every test scenario
    # CI pipeline might fail although all tests are passing
    coredumps = getCoredumps()
    if coredumps:
        try:
            generateStacktrace(context, coredumps)
            test.log("Stacktrace generated!")
        except Exception as err:
            test.log("Exception occured:" + str(err))
    else:
        test.log("No coredump found!")

    # cleanup test server
    req = urllib.request.Request(
        os.path.join(context.userData['middlewareUrl'], 'cleanup'),
        headers={"Content-Type": "application/json"},
        method='POST',
    )
    try:
        urllib.request.urlopen(req)
    except urllib.error.HTTPError as e:
        raise Exception(
            "Step execution through test middleware failed. Error: " + e.read().decode()
        )

    previousFailResultCount = test.resultCount("fails")
    previousErrorResultCount = test.resultCount("errors")
