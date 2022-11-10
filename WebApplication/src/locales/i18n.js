import { createI18n } from "vue-i18n";
import en from "./en.json";
import es from "./es.json";
import fr from "./fr.json";
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
// import zh from "./zh.json";
import { localeConf } from "../globalConfigurations"

const i18n = createI18n({
  locale: localeConf,
  fallbackLocale: 'en',  
  messages: {
    en,
    es,
    fr,
    // it,
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

export default i18n;