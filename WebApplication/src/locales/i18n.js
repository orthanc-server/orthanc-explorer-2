import { createI18n } from "vue-i18n";
import en from "./en.json";
import de from "./de.json";
import es from "./es.json";
import fr from "./fr.json";
import it from "./it.json";
import ka from "./ka.json";
import ru from "./ru.json";
import si from "./si.json";
import uk from "./uk.json";
import zh from "./zh.json";

const i18n = createI18n({
  warnHtmlInMessage: 'off',
  locale: 'en',    // when the list of availableLanguages is loaded, this value is updated in LanguagePicker.isConfigurationLoaded
  fallbackLocale: 'en',  
  messages: {
    en,
    de,
    es,
    fr,
    it,
    ka,
    ru,
    si,
    uk,
    zh
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
