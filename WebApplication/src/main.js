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

const app = createApp(App)

app.use(router)
app.use(store)
app.use(i18n)


app.mount('#app')