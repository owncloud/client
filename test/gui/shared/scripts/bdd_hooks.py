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
import os
from datetime import datetime

from helpers.StacktraceHelper import get_core_dumps, generate_stacktrace
from helpers.SyncHelper import close_socket_connection, clear_waited_after_sync
from helpers.SpaceHelper import delete_project_spaces
from helpers.api.provisioning import delete_created_groups, delete_created_users
from helpers.SetupClientHelper import wait_until_app_killed
from helpers.ConfigHelper import (
    init_config,
    get_config,
    set_config,
    clear_scenario_config,
    is_windows,
    is_linux,
)
from helpers.FilesHelper import prefix_path_namespace, cleanup_created_paths
from helpers.ReportHelper import save_video_recording, take_screenshot, is_video_enabled
import helpers.api.oc10 as oc

from pageObjects.Toolbar import Toolbar
from pageObjects.AccountSetting import AccountSetting
from pageObjects.AccountConnectionWizard import AccountConnectionWizard

# Squish test settings:
# This controls whether a test (scenario) should stop execution on failure or not
# If set to True, the scenario will stop on the first step failure and remaining steps will not be executed
# If set to False, the scenario will continue to execute all steps and report all failures at the end
testSettings.throwOnFailure = True

# this will reset in every test suite
PREVIOUS_FAIL_RESULT_COUNT = 0
PREVIOUS_ERROR_RESULT_COUNT = 0


# runs before a feature
# Order: 1
@OnFeatureStart
def hook(context):
    init_config()


# runs before every scenario
# Order: 1
@OnScenarioStart
def hook(context):
    clear_scenario_config()


# runs before every scenario
# Order: 2
@OnScenarioStart
def hook(context):
    # set owncloud config file path
    config_dir = get_config("clientConfigDir")
    if os.path.exists(config_dir):
        if len(os.listdir(config_dir)) and is_windows():
            raise FileExistsError(
                "Looks like you have previous client config in '"
                + config_dir
                + "'\n[DANGER] Delete it and try again."
                + "\n[DANGER] Removing config file will make client to lost the previously added accounts."
            )
        # clean previous configs
        shutil.rmtree(config_dir)
    os.makedirs(config_dir, 0o0755)
    set_config("clientConfigFile", os.path.join(config_dir, "owncloud.cfg"))

    # create reports dir if not exists
    test_report_dir = get_config("guiTestReportDir")
    if not os.path.exists(test_report_dir):
        os.makedirs(test_report_dir)

    # log tests scenario title on serverlog file
    if os.getenv("CI"):
        with open(test_report_dir + "/serverlog.log", "a", encoding="utf-8") as f:
            f.write(
                str((datetime.now()).strftime("%H:%M:%S:%f"))
                + "\tBDD Scenario: "
                + context.title
                + "\n"
            )

    # this path will be changed according to the user added to the client
    # e.g.: /tmp/client-bdd/Alice
    set_config("currentUserSyncPath", "")

    root_sync_dir = get_config("clientRootSyncPath")
    if not os.path.exists(root_sync_dir):
        os.makedirs(root_sync_dir)

    tmp_dir = get_config("tempFolderPath")
    if not os.path.exists(tmp_dir):
        os.makedirs(tmp_dir)

    # sync connection folder display name
    set_config("syncConnectionName", "Personal" if get_config("ocis") else "ownCloud")


# determines if the test scenario failed or not
# Currently, this workaround is needed because we cannot find out
#   a way to determine the pass/fail status of currently running test scenario.
# And, resultCount("errors")  and resultCount("fails")
#   return the total number of error/failed test scenarios of a test suite.
def scenario_failed():
    return (
        test.resultCount("fails") - PREVIOUS_FAIL_RESULT_COUNT > 0
        or test.resultCount("errors") - PREVIOUS_ERROR_RESULT_COUNT > 0
    )


def scenario_title_to_filename(title):
    # scenario name can have "/" which is invalid filename
    return title.replace(" ", "_").replace("/", "_").strip(".")


# runs after every scenario
# Order: 1
@OnScenarioEnd
def hook(context):
    clear_waited_after_sync()
    close_socket_connection()

    # generate screenshot and video reports
    if is_linux():
        filename = scenario_title_to_filename(context.title)
        if scenario_failed():
            take_screenshot(f"{filename}.png")

        if is_video_enabled():
            save_video_recording(f"{filename}.mp4", scenario_failed())

    # teardown accounts and configs
    teardown_client()

    # search coredumps after every test scenario
    # CI pipeline might fail although all tests are passing
    if coredumps := get_core_dumps():
        try:
            generate_stacktrace(context.title, coredumps)
            test.log("Stacktrace generated!")
        except OSError as err:
            test.log("Exception occured:" + str(err))
    elif scenario_failed():
        test.log("No coredump found!")

    global PREVIOUS_FAIL_RESULT_COUNT, PREVIOUS_ERROR_RESULT_COUNT
    PREVIOUS_FAIL_RESULT_COUNT = test.resultCount("fails")
    PREVIOUS_ERROR_RESULT_COUNT = test.resultCount("errors")


# runs after every scenario
# Order: 2
# server cleanup
@OnScenarioEnd
def hook(context):
    if get_config("ocis"):
        delete_project_spaces()
    else:
        oc.restore_apps_state()
    delete_created_groups()
    delete_created_users()


def teardown_client():
    # Cleanup user accounts from UI for Windows platform
    # It is not needed for Linux so skipping it in order to save CI time
    if is_windows():
        # remove account from UI
        # In Windows, removing only config and sync folders won't help
        # so to work around that, remove the account connection
        close_dialogs()
        close_widgets()
        accounts, _ = Toolbar.get_accounts()
        for account in accounts:
            Toolbar.open_account(account["displayname"])
            AccountSetting.remove_account_connection()
        if accounts:
            squish.waitForObject(AccountConnectionWizard.SERVER_ADDRESS_BOX)

    # Detach (i.e. potentially terminate) all AUTs at the end of a scenario
    for ctx in squish.applicationContextList():
        # get pid before detaching
        pid = ctx.pid
        ctx.detach()
        wait_until_app_killed(pid)

    # clean up config files
    shutil.rmtree(get_config("clientConfigDir"))

    # delete test files/folders
    for entry in os.scandir(get_config("clientRootSyncPath")):
        try:
            if entry.is_file() or entry.is_symlink():
                test.log("Deleting file: " + entry.name)
                os.unlink(prefix_path_namespace(entry.path))
            elif entry.is_dir():
                test.log("Deleting folder: " + entry.name)
                shutil.rmtree(prefix_path_namespace(entry.path))
        except OSError as e:
            test.log(f"Failed to delete '{entry.name}'.\nReason: {e}.")
    # cleanup paths created outside of the temporary directory during the test
    cleanup_created_paths()


def close_dialogs():
    # close the current active dailog if it's not a main client window
    while True:
        active_window = QApplication.activeModalWidget()
        if str(active_window) == "<null>":
            break
        test.log(f"Closing '{active_window.objectName}' window")
        if not active_window.close():
            confirm_dialog = QApplication.activeModalWidget()
            if confirm_dialog.visible:
                squish.clickButton(
                    squish.waitForObject(AccountSetting.CONFIRMATION_YES_BUTTON)
                )


def close_widgets():
    try:
        ch = object.children(squish.waitForObject(AccountSetting.DIALOG_STACK, 500))
        for obj in ch:
            if (
                hasattr(obj, "objectName")
                and obj.objectName
                and obj.objectName != "page"
            ):
                obj.close()
                # if the dialog has a confirmation dialog, confirm it
                confirm_dialog = QApplication.activeModalWidget()
                if str(confirm_dialog) != "<null>" and confirm_dialog.visible:
                    squish.clickButton(
                        squish.waitForObject(AccountSetting.CONFIRMATION_YES_BUTTON)
                    )
    except LookupError:
        # nothing to close if DIALOG_STACK is not found
        # required for client versions <= 5
        pass
