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
import Keycloak from "keycloak-js"

const app = createApp(App)

app.use(router)
app.use(store)
app.use(i18n)


// This part of the code allows to delegate authentication to Keycloak

// TODO: make this keycloak code optional
let initOptions = {
    // TODO: replace 'url', 'orthanc-realm' and 'clientId' values by env var
    url: 'http://localhost:8080/', realm: 'orthanc-realm', clientId: 'orthanc-id', onLoad:'login-required'
  }
  
  let keycloak = Keycloak(initOptions);
  
  keycloak.init({ onLoad: initOptions.onLoad }).success((auth) =>{
      
      if(!auth) {
        window.location.reload();
      } else {
        console.log("Authenticated");
      }
   
      app.mount('#app')
     
      localStorage.setItem("vue-token", keycloak.token);
      localStorage.setItem("vue-refresh-token", keycloak.refreshToken);
  
      setInterval(() =>{
        keycloak.updateToken(70).success((refreshed)=>{
          if (refreshed) {
            console.log('Token refreshed'+ refreshed);
          } else {
            console.log('Token not refreshed, valid for '
            + Math.round(keycloak.tokenParsed.exp + keycloak.timeSkew - new Date().getTime() / 1000) + ' seconds');
          }
        }).error(()=>{
            console.log('Failed to refresh token');
        });
  
  
      }, 60000)
  
  }).error(() =>{
    console.log("Authenticated Failed");
  });