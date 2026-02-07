<script>
import allLanguages from '../store/modules/i18n';
import { mapState } from "vuex"

export default {
    name: 'LanguageSwitcher',
    props: ['isCollapsed'],
    data() {
        return {
            availableLanguages: ['en', 'ru'],
            currentLanguageIndex: 0
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
        currentLanguage() {
            const key = this.availableLanguages[this.currentLanguageIndex];
            return allLanguages.find(lang => lang.key === key) || { key: 'en', name: 'English' };
        }
    },
    watch: {
        isConfigurationLoaded(newValue) {
            if (newValue && this.uiOptions.AvailableLanguages) {
                const allowedLanguages = ['en', 'ru'];
                const configLanguages = this.uiOptions.AvailableLanguages || [];

                this.availableLanguages = configLanguages.filter(lang =>
                    allowedLanguages.includes(lang)
                );

                if (this.availableLanguages.length === 0) {
                    this.availableLanguages = ['en', 'ru'];
                }

                const savedLang = localStorage.getItem("OE2.languageKey");
                const index = this.availableLanguages.indexOf(savedLang);
                this.currentLanguageIndex = index !== -1 ? index : 0;

                const langToApply = this.availableLanguages[this.currentLanguageIndex];
                this.applyLanguage(langToApply, false);
            }
        }
    },
    methods: {
        toggleLanguage() {
            this.currentLanguageIndex = (this.currentLanguageIndex + 1) % this.availableLanguages.length;
            const newLang = this.availableLanguages[this.currentLanguageIndex];
            this.applyLanguage(newLang, true);
        },
        applyLanguage(key, emitEvent = true) {
            this.$i18n.locale = key;
            localStorage.setItem("OE2.languageKey", key);
            if (emitEvent && this.messageBus) {
                this.messageBus.emit('language-changed', key);
            }
        }
    }
}
</script>

<template>
    <button v-if="showLanguagePicker" class="menu-item language-switcher"
        :class="{ 'language-switcher--collapsed': isCollapsed }" @click="toggleLanguage" type="button"
        :title="isCollapsed ? currentLanguage.name : ''">
        <i class="bi bi-globe"></i>
        <span v-if="!isCollapsed">{{ currentLanguage.name }}</span>
    </button>
</template>

<style scoped>
.menu-item.language-switcher {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 10px 12px;
    border-radius: 4px;
    cursor: pointer;
    transition: all 0.2s ease;
    color: #94a3b8;
    font-size: 13px;
    font-weight: 400;
    background: transparent;
    border: none;
    width: 100%;
    text-align: left;
    font-family: inherit;
}

.menu-item.language-switcher:hover {
    background: rgba(255, 255, 255, 0.05);
    color: #e2e8f0;
    transform: translateX(4px);
}

.menu-item.language-switcher i {
    width: 20px;
    text-align: center;
    font-size: 14px;
    transition: transform 0.2s ease;
}

.menu-item.language-switcher:hover i {
    transform: scale(1.1) rotate(15deg);
    color: #3b82f6;
}

/* Стили для collapsed режима */
.language-switcher--collapsed {
    justify-content: center;
    padding: 10px 0;
}

.language-switcher--collapsed i {
    width: auto;
}

.collapsed .language-switcher {
    bottom: 50px !important;
    position: relative;
}

.collapsed .language-switcher:hover {
    bottom: 50px !important;
    position: relative;
    transform: scale(1.0);
}
</style>