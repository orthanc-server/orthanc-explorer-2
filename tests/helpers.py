from playwright.sync_api import sync_playwright, Page, expect


def get_collapsed_studies(page: Page):
    return page.locator(".study-row-collapsed")

def get_date_picker_value(page: Page, id: str):
    return page.locator(f"{id} input").first.input_value()
