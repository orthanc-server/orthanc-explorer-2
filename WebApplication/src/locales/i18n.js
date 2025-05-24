import { createI18n } from "vue-i18n";
import en from "./en.json";
import ar from "./ar.json";
import de from "./de.json";
import es from "./es.json";
import fr from "./fr.json";
import it from "./it.json";
import ka from "./ka.json";
import ro from "./ro.json";
import ru from "./ru.json";
import si from "./si.json";
import uk from "./uk.json";
import vi from "./vi.json";
import zh from "./zh.json";
import pt from "./pt.json";

const i18n = createI18n({
  warnHtmlInMessage: 'off',
  locale: 'en',    // when the list of availableLanguages is loaded, this value is updated in LanguagePicker.isConfigurationLoaded
  fallbackLocale: 'en',  
  messages: {
    en,
    ar,
    de,
    es,
    fr,
    it,
    ka,
    ro,
    ru,
    si,
    uk,
    vi,
    zh,
	pt,
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
