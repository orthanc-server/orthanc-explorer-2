import { createRouter, createWebHistory } from 'vue-router'
import Settings from './components/Settings.vue'
import StudyList from './components/StudyList.vue'
import SideBar from './components/SideBar.vue'
import NotFound from './components/NotFound.vue'
import { baseOe2Url } from "./globalConfigurations"

console.log('Base URL for router: ', baseOe2Url);

export const router = createRouter({
  history: createWebHistory(baseOe2Url),
  routes: [
    {
      path: '/',
      alias: '/index.html',
      components: {
        SideBarView: SideBar,
        ContentView: StudyList,
      },
    },
    {
      path: '/filtered-studies/:filters+',
      components: {
        SideBarView: SideBar,
        ContentView: StudyList,
      },
    },
    {
      path: '/settings',
      components: {
        SideBarView: SideBar,
        ContentView: Settings,
      },
    },
    {
      path: '/:pathMatch(.*)',
      components: {
        SideBarView: SideBar,
        ContentView: NotFound,
      },
    }

  ],
})
