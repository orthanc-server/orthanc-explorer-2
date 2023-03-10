import { createApp } from 'vue'
import AppRetrieveAndView from './AppRetrieveAndView.vue'
import i18n from './locales/i18n'
import "bootstrap/dist/css/bootstrap.min.css"
import "bootstrap/dist/js/bootstrap.bundle.min.js"
import "bootstrap-icons/font/bootstrap-icons.css"
import "@fortawesome/fontawesome-free/css/all.min.css"
import store from "./store"
import orthancApi from './orthancApi'
import axios from 'axios'



// before initialization, we must load part of the configuration to know if we need to enable Keycloak or not
axios.get('../api/pre-login-configuration').then((config) => {

  const app = createApp(AppRetrieveAndView)

  app.use(store)
  app.use(i18n)

  app.mount('#app-retrieve-and-view')

});
