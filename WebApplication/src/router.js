import { createRouter, createWebHashHistory } from 'vue-router'
import Settings from './components/Settings.vue'
import SettingsLabels from './components/SettingsLabels.vue'
import SettingsPermissions from './components/SettingsPermissions.vue'
import Worklists from './components/Worklists.vue'
import AuditLogs from './components/AuditLogs.vue'
import StudyList from './components/StudyList.vue'
import SideBar from './components/SideBar.vue'
import NotFound from './components/NotFound.vue'
import { baseOe2Url } from "./globalConfigurations"

console.log('Base URL for router: ', baseOe2Url);

function removeKeyCloakStates(to, from, next) {
  if (to.path.includes("&state=")) {
    // the router does not recognize the &state=... after a redirect from KeyCloak -> simply block it (it works but I don't really understand why !)
    console.log("removeKeyCloakStates", to, from);
    next(false);
  } else {
    next();
  }
}

export const router = createRouter({
  history: createWebHashHistory(baseOe2Url),
  routes: [
    {
      path: '/',
      alias: '/index.html',
      components: {
        SideBarView: SideBar,
        ContentView: StudyList,
      },
      name: 'home'
    },
    {
      path: '/filtered-studies',
      components: {
        SideBarView: SideBar,
        ContentView: StudyList,
      },
      name: 'local-studies-list'
    },
    {
      path: '/worklists',
      components: {
        SideBarView: SideBar,
        ContentView: Worklists,
      },
      name: 'worklists'
    },
    {
      path: '/settings',
      components: {
        SideBarView: SideBar,
        ContentView: Settings,
      },
      name: 'settings'
    },
    {
      path: '/settings-labels',
      components: {
        SideBarView: SideBar,
        ContentView: SettingsLabels,
      },
      name: 'settings-labels'
    },
    {
      path: '/settings-permissions',
      components: {
        SideBarView: SideBar,
        ContentView: SettingsPermissions,
      },
      name: 'settings-permissions'
    },
    {
      path: '/audit-logs',
      components: {
        SideBarView: SideBar,
        ContentView: AuditLogs,
      },
      name: 'audit-logs'
    },
    {
      path: '/:pathMatch(.*)',
      beforeEnter: removeKeyCloakStates,
      components: {
        SideBarView: SideBar,
        ContentView: NotFound,
      },
      name: 'keycloak-path-match'
    }

  ],
})
