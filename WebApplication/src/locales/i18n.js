import { createI18n } from "vue-i18n";
import en from "./en.json";
import es from "./es.json";
import fr from "./fr.json";
import it from "./it.json";
import ka from "./ka.json";
import uk from "./uk.json";
import zh from "./zh.json";
// import it from "./it.json";
// import pt from "./pt.json";
// import ru from "./ru.json";
// import de from "./de.json";
// import nl from "./nl.json";
// import jp from "./jp.json";
// import bn from "./bn.json";
// import xn from "./in.json";
// import hi from "./hi.json";
// import sw from "./sw.json";

const i18n = createI18n({
  warnHtmlInMessage: 'off',
  locale: 'en',    // when the list of availableLanguages is loaded, this value is updated in LanguagePicker.isConfigurationLoaded
  fallbackLocale: 'en',  
  messages: {
    en,
    es,
    fr,
    it,
    ka,
    uk,
    zh
    // pt,
    // ru,
    // de,
    // nl,
    // jp,
    // xn,
    // bn,
    // hi,
    // sw,
    // zh,
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
