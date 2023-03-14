<script>
import allLanguages from '../store/modules/i18n';
import { mapState } from "vuex"

export default {
    name: 'App',
    props: [],
    data() {
        return {
            availableLanguages: ['en'], // before the configuration is loaded, we consider that there is only Engligh
            selectedLanguageName: ''
        };
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            isConfigurationLoaded: state => state.configuration.loaded,
        }),
        showLanguagePicker() {
            return this.availableLanguages.length > 1;
        },
        pickableLanguages() {
            if (this.availableLanguages == null) {
                return allLanguages;
            } else {
                let languages = [];
                for (let languageCode of this.availableLanguages) {
                    for (let lang of allLanguages) {
                        if (lang['key'] == languageCode) {
                            languages.push(lang);
                        }
                    }
                }

                return languages;
            }
        }
    },
    watch: {
        isConfigurationLoaded(newValue, oldValue) {
            this.availableLanguages = this.uiOptions.AvailableLanguages;
            let done = false;

            if (this.availableLanguages.length == 1) {
                let languageKey = this.availableLanguages[0];
                console.log("language picker: selecting first and only language: ", languageKey)
                done = this.changeLanguage(languageKey);
            } else if (this.availableLanguages.length > 1) {

                if (localStorage.getItem("OE2.languageKey") != null) {
                    console.log("language picker: selecting language from the local storage");
                    done = this.changeLanguage(localStorage.getItem("OE2.languageKey"));
                }

                if (!done && this.uiOptions.DefaultLanguage != null) {
                    console.log("language picker: selecting language from DefaultLanguage configuration");
                    done = this.changeLanguage(this.uiOptions.DefaultLanguage);
                }

                if (!done) {
                    console.log("language picker: selecting language from the browser languages");
                    if (navigator.languages && navigator.languages.length > 0) {
                        for (let lang of navigator.languages) {
                            let langCode = lang.split('-')[0];
                            done = this.changeLanguage(langCode);
                            if (done) {
                                break;
                            }
                        }
                    }
                }
                if (!done) {
                    done = this.changeLanguage(this.availableLanguages[0]);
                }
            }
        },
    },
    created() {
        this.selectedLanguageName = 'English'
    },
    methods: {
        changeLanguage(key) {
            let languageName = null;

            for (let lang of allLanguages) {
                if (lang['key'] == key) {
                    languageName = lang['name'];
                }
            }

            if (languageName != null) {
                this.$i18n.locale = key;
                this.selectedLanguageName = languageName;

                localStorage.setItem("OE2.languageKey", key);
                this.messageBus.emit('language-changed', key);
                return true;
            } else {
                return false;
            }
        },
    },
}
</script>

<template>
    <div class="dropdown" v-if="showLanguagePicker">
        <button class="btn btn-sm btn-secondary dropdown-toggle" type="button" id="dropdownMenuButton1" data-bs-toggle="dropdown"
            aria-expanded="false">
            <i class="bi bi-translate"></i> {{ selectedLanguageName }}
        </button>
        <ul class="dropdown-menu language-picker">
            <li v-for="(lang, i) in pickableLanguages" :key="`lang-${i}`" :value="lang"><a class="dropdown-item" href="#"
                    @click="changeLanguage(`${lang.key}`)">{{ lang.name }}</a></li>
        </ul>
    </div>
</template>

<style>
.language-picker {
    margin-top: var(--details-top-margin);
    font-size: 0.9rem;
    --bs-dropdown-item-padding-y: 0.05rem;
}
</style>
