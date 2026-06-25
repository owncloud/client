# -*- coding: utf-8 -*-
#
# Step definitions for the on-demand documentation screenshot suite
# (tst_docScreenshots). Not used by the regular GUI tests.

from helpers.DocScreenshotHelper import capture_doc_screenshot
from pageObjects.DocScreens import DocScreens
from pageObjects.SyncConnection import SyncConnection


@Then('the screenshot "|any|" is captured')
def step(context, screen_id):
    capture_doc_screenshot(screen_id)


@When('the user opens the settings tab')
def step(context):
    DocScreens.open_settings_tab()


@When('the user opens the ignored files editor')
def step(context):
    DocScreens.open_ignored_files_editor()


@When('the user opens the log settings window')
def step(context):
    DocScreens.open_log_settings()


@When('the user opens the folder sync options menu')
def step(context):
    SyncConnection.open_menu()
