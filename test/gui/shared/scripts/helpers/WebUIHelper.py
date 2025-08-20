import squish
from playwright.sync_api import sync_playwright

from helpers.ConfigHelper import get_config

envs = {}


def get_clipboard_text():
    try:
        return squish.getClipboardText()
    except:
        # Retry after 2 seconds
        squish.snooze(2)
        return squish.getClipboardText()


def authorize_via_webui(username, password):
    global envs
    envs = {
        'OC_USERNAME': username.strip('"'),
        'OC_PASSWORD': password.strip('"'),
        'OC_AUTH_URL': get_clipboard_text(),
    }

    with sync_playwright() as p:
        browser = p.chromium.launch()
        context = browser.new_context(ignore_https_errors=True)
        page = context.new_page()
        page.goto(envs['OC_AUTH_URL'])
        oidc_login(page)
        context.close()
        browser.close()


def oidc_login(page):
    if get_config('client_name') != 'ownCloud':
        page.fill('#email', envs['OC_USERNAME'])
        page.click('span:text-is("Next")')
        page.fill('#password', envs['OC_PASSWORD'])
        page.click('span:text-is("Sign in")')
        page.wait_for_selector('text=Login Successful')
    else:
        # login
        page.fill('#oc-login-username', envs['OC_USERNAME'])
        page.fill('#oc-login-password', envs['OC_PASSWORD'])
        page.click('button[type=submit]')
        # allow permissions
        page.click('button >> text=Allow')
        # confirm successful login
        page.wait_for_selector('text=Login Successful')
