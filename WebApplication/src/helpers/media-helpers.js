// keep in sync with media.scss
export const BREAKPOINTS = { narrowMax: 767, wideMax: 1300 };
export const MOBILE_ANY = `(max-width: ${BREAKPOINTS.wideMax}px)`;
export const MOBILE_NARROW = `(max-width: ${BREAKPOINTS.narrowMax}px), (orientation: portrait) and (max-width: ${BREAKPOINTS.wideMax}px)`;
export const MOBILE_WIDE = `(min-width: ${BREAKPOINTS.narrowMax + 1}px) and (max-width: ${BREAKPOINTS.wideMax}px) and (orientation: landscape)`;

import { ref, onMounted, onBeforeUnmount } from 'vue';

export function useMediaQuery(query) {
  const matches = ref(false);
  let mql, listener;

  onMounted(() => {
    mql = window.matchMedia(query);
    matches.value = mql.matches;
    listener = (e) => { matches.value = e.matches; };
    mql.addEventListener('change', listener);
  });

  onBeforeUnmount(() => {
    if (mql && listener) mql.removeEventListener('change', listener);
  });

  return matches;
}

export function useIsMobileAny() {
    return useMediaQuery(MOBILE_ANY);
}

export function useIsMobileNarrow() {
    return useMediaQuery(MOBILE_NARROW);
}

export function useIsMobileWide() {
    return useMediaQuery(MOBILE_WIDE);
}

export function isDarkMode() {
    // hack to switch the theme: get the value from our custom css
    let bootstrapTheme = document.documentElement.getAttribute("data-bs-theme"); // for production
    bootstrapTheme = getComputedStyle(document.documentElement).getPropertyValue('--bootstrap-theme');  // for dev
    // console.log("DatePicker color mode is ", bootstrapTheme);
    return bootstrapTheme == "dark";

}