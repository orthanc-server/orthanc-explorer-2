import { createApp } from 'vue'
import AppLanding from './AppLanding.vue'
import i18n from './locales/i18n'
import "bootstrap/dist/css/bootstrap.min.css"
import "bootstrap/dist/js/bootstrap.bundle.min.js"
import "bootstrap-icons/font/bootstrap-icons.css"
import "@fortawesome/fontawesome-free/css/all.min.css"
import store from "./store"
import orthancApi from './orthancApi'
import axios from 'axios'


// Names of the params that can contain an authorization token
// If one of these params contain a token, it will be passed as a header
// with each request to the Orthanc API
const VALID_TOKEN_PARAMS = ["token", "auth-token", "authorization"];


// before initialization, we must load part of the configuration to know if we need to enable Keycloak or not
axios.get('../api/pre-login-configuration').then((config) => {

  const app = createApp(AppLanding)
  
  app.config.globalProperties.$appConfig = {'TokensLandingOptions': config.data['TokensLandingOptions']};

  app.use(store)
  app.use(i18n)

  // If there is a param with a token in the params, use it as a header in subsequent calls to the Orthanc API
  const params = new URLSearchParams(window.location.search);

  for (let paramName of VALID_TOKEN_PARAMS) {
      const paramValue = params.get(paramName);

      if (!paramValue) continue;

      localStorage.setItem(paramName, paramValue);
      orthancApi.updateAuthHeader(paramName);
  }

  app.mount('#app-landing')

});
