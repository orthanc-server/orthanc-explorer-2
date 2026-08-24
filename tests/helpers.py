from playwright.sync_api import sync_playwright, Page, expect


def get_collapsed_studies(page: Page):
    return page.locator(".study-row-collapsed")

