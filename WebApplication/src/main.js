import { createApp } from 'vue'
import App from './App.vue'
import i18n from './locales/i18n'
import "bootstrap/dist/css/bootstrap.min.css"
import "bootstrap/dist/js/bootstrap.bundle.min.js"
import "bootstrap-icons/font/bootstrap-icons.css"
import "@fortawesome/fontawesome-free/css/all.min.css"
import "./assets/css/layout.css"
import "./assets/css/common.css"
import store from "./store"
import { router } from './router'
import Keycloak from "keycloak-js"
import orthancApi from './orthancApi'
import axios from 'axios'
import Datepicker from '@vuepic/vue-datepicker';
import '@vuepic/vue-datepicker/dist/main.css';
import mitt from "mitt"
import VueObserveVisibility from 'vue3-observe-visibility'
import { baseOe2Url } from './globalConfigurations.js'

// Names of the params that can contain an authorization token
// If one of these params contain a token, it will be passed as a header
// with each request to the Orthanc API
const VALID_TOKEN_PARAMS = ["token", "auth-token", "authorization"];

// before initialization, we must load part of the configuration to know if we need to enable Keycloak or not
axios.get('../api/pre-login-configuration').then((config) => {

    const app = createApp(App)
    const messageBus = mitt();

    // uncomment this code to log messages
    // const originalEmit = messageBus.emit;
    // messageBus.emit = (event, payload) => {
    //     console.log(`[EVENT BUS - EMIT] ${event} `, payload);
    //     originalEmit.call(messageBus, event, payload);
    // }

    // const originalOn = messageBus.on;
    // messageBus.on = (event, handler) => {
    //     const wrappedHandler = (payload) => {
    //         console.log(`[EVENT BUS - HANDLER] ${event} `, payload);
    //         handler(payload);
    //       };
    //     originalOn.call(messageBus, event, wrappedHandler);
    // }

    app.use(router)
    app.use(store)
    app.use(i18n)
    app.use(VueObserveVisibility)
    app.component('Datepicker', Datepicker);

    app.config.globalProperties.messageBus = messageBus;

    let keycloackConfig = null;

    if (config.data['Keycloak'] && config.data['Keycloak'] != null && config.data['Keycloak']['Enable']) {
        console.log("Keycloak is enabled");

        keycloackConfig = config.data['Keycloak']

        let initOptions = {
            url: keycloackConfig['Url'],
            realm: keycloackConfig['Realm'],
            clientId: keycloackConfig['ClientId'],
            onLoad: 'login-required'
        }

        window.keycloak = new Keycloak(initOptions);

        window.keycloak.init({ onLoad: initOptions.onLoad }).then(async (auth) => {

            if (!auth) {
                window.location.reload();
            } else {
                console.log("Authenticated");
            }

            localStorage.setItem("vue-token", window.keycloak.token);
            localStorage.setItem("vue-refresh-token", window.keycloak.refreshToken);
            orthancApi.updateAuthHeader();

            { // remove the keycloak uri arguments that are passed after a hash (ex: #state=1234&iss=...)
                let fullLocation = window.location.href;
                // since we have removed the hash router in 1.13.0, remove the '#/' in case someone is still using an old link like ../ui/app/#/filtered-studies?PatientID=1234
                fullLocation = fullLocation.replace('/ui/app/#/', '/ui/app/');

                const actualUrl = new URL(fullLocation);
                const actualPathAndQuery = `${actualUrl.pathname}${actualUrl.search}`;
                const currentFullPath = router.currentRoute.value.fullPath;
                
                // If the fullPath includes a hash with Keycloak parameters, remove it
                const cleanedFullPath = currentFullPath.split('#')[0];

                // If the cleanedFullPath does not match the actual URL, update the route
                if (cleanedFullPath !== actualPathAndQuery) {
                    // Replace the current route with the actual URL path and query
                    await router.replace(actualPathAndQuery.replace(baseOe2Url, ""));
                }            
            }

            app.mount('#app');
            console.log("App mounted with keycloak, current route is ", currentPath);

            // programm token refresh at regular interval
            setInterval(() => {
                window.keycloak.updateToken(70).then((refreshed) => {
                    if (refreshed) {
                        console.log('Token refreshed');
                        localStorage.setItem("vue-token", window.keycloak.token);
                        localStorage.setItem("vue-refresh-token", window.keycloak.refreshToken);
                        orthancApi.updateAuthHeader();
                    } else {
                        console.log('Token not refreshed, still valid for '
                            + Math.round(window.keycloak.tokenParsed.exp + window.keycloak.timeSkew - new Date().getTime() / 1000) + ' seconds');
                    }
                }).catch(() => {
                    console.log('Failed to refresh token');
                });

            }, 60000)

        }).catch((e) => {
            console.log("Could not connect to Keycloak", e);
        });
    } else {
        router.isReady().then(() => {
            console.log("opening OE2 with router=", router.currentRoute.value);
            // If there is a param with a token in the params, use it as a header in subsequent calls to the Orthanc API

            for (let paramName of VALID_TOKEN_PARAMS) {

                if (paramName in router.currentRoute.value.query) {
                    const paramValue = router.currentRoute.value.query[paramName];
                    if (!paramValue) {
                        continue;
                    }

                    console.log("found auth token in url: ", paramName, paramValue);
                    localStorage.setItem(paramName, paramValue);
                    orthancApi.updateAuthHeader(paramName);
                }
            }

            app.mount('#app');
        });
    }
});
