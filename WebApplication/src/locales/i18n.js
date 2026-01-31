import { createI18n } from "vue-i18n";
import en from "./en.json";
import ru from "./ru.json";

const i18n = createI18n({
  warnHtmlInMessage: 'off',
  locale: 'en',    // when the list of availableLanguages is loaded, this value is updated in LanguagePicker.isConfigurationLoaded
  fallbackLocale: 'en',  
  messages: {
    en,
    ru,
  },
});

document._mustTranslateDicomTags = false;

function translateDicomTag($t, $te, tagName) {
  if (document._mustTranslateDicomTags) {
    if ($te('dicom_tags.' + tagName)) {
      return $t('dicom_tags.' + tagName)
    } else {
      return tagName;
    }
  } else {
    return tagName;
  }
}

export {
  i18n as default,
  translateDicomTag
};
