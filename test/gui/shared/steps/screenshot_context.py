# -*- coding: utf-8 -*-
#
# Step definitions for the on-demand documentation screenshot suite
# (tst_docScreenshots). Not used by the regular GUI tests.

from helpers.DocScreenshotHelper import capture_doc_screenshot


@Then('the screenshot "|any|" is captured')
def step(context, screen_id):
    capture_doc_screenshot(screen_id)
