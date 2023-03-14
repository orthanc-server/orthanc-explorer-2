import { createApp } from 'vue'
import App from './App.vue'
import i18n from './locales/i18n'
import "bootstrap/dist/css/bootstrap.min.css"
import "bootstrap/dist/js/bootstrap.bundle.min.js"
import "bootstrap-icons/font/bootstrap-icons.css"
import "@fortawesome/fontawesome-free/css/all.min.css"
import "./assets/css/_variables.css"
import "./assets/css/layout.css"
import "./assets/css/common.css"
import store from "./store"
import { router } from './router'
import Datepicker from '@vuepic/vue-datepicker';
import '@vuepic/vue-datepicker/dist/main.css';
import mitt from "mitt"

const app = createApp(App)
const messageBus = mitt();

app.use(router)
app.use(store)
app.use(i18n)
app.component('Datepicker', Datepicker);

app.config.globalProperties.messageBus = messageBus;

app.mount('#app')