<script>
import MobileBottomMenu from "./components/MobileBottomMenu.vue";
import MobileFiltersModal from "./components/MobileFiltersModal.vue";

function applyBootStrapTheme() {
  let bootstrapTheme = getComputedStyle(
    document.documentElement,
  ).getPropertyValue("--bootstrap-theme");
  console.log("-------------- Applying Bootstrap theme ...", bootstrapTheme);
  if (bootstrapTheme) {
    document.documentElement.setAttribute("data-bs-theme", bootstrapTheme);
  } else {
    console.warn(
      "-------------- Applying Bootstrap theme not defined yet, retrying ...",
    );
    setTimeout(applyBootStrapTheme, 100000);
  }
}

export default {
  components: {
    MobileBottomMenu,
    MobileFiltersModal,
  },
  data() {
    return {
      sidebarCollapsed: false,
      isMobilePortrait: false,
      showMobileFilters: false
    };
  },
  computed: {
    currentStudyDate() {
      return this.$store.state.studies.dicomTagsFilters?.StudyDate || "";
    }
  },
  async created() {
    console.log("Creating App...");

    {
      console.log("Loading the defaults + custom CSS ...");
      let link = document.createElement("link");
      link.rel = "stylesheet";
      link.type = "text/css";
      link.href = "customizable/custom.css";
      document.getElementsByTagName("HEAD")[0].appendChild(link);
      setTimeout(applyBootStrapTheme, 0);
    }

    await this.$store.dispatch("configuration/load");
    await this.$store.dispatch("studies/initialLoad");
    await this.$store.dispatch("labels/refresh");
    console.log("App created");

    const savedState = localStorage.getItem("sidebarCollapsed");
    if (savedState !== null) {
      this.sidebarCollapsed = savedState === "true";
    }

    this.checkMobilePortrait();
    window.addEventListener("resize", this.checkMobilePortrait);
    window.addEventListener("orientationchange", this.checkMobilePortrait);
    window.addEventListener("sidebar-collapsed", this.handleSidebarToggle);
  },
  beforeUnmount() {
    window.removeEventListener("sidebar-collapsed", this.handleSidebarToggle);
    window.removeEventListener("resize", this.checkMobilePortrait);
    window.removeEventListener("orientationchange", this.checkMobilePortrait);
  },
  methods: {
    openMobileFilters() {
      this.showMobileFilters = true;
    },

    closeMobileFilters() {
      this.showMobileFilters = false;
    },

    onMobileFiltersApplied(appliedFilters) {
      this.showMobileFilters = false;
      // Эмитим событие для StudyList
      if (this.$root.messageBus) {
        this.$root.messageBus.emit('filters-applied-from-mobile', appliedFilters);
      }
    },

    onMobileNavigate(target) {
      console.log('Mobile navigation to:', target);
    },

    handleSidebarToggle(e) {
      if (!this.isMobilePortrait) {
        this.sidebarCollapsed = e.detail.collapsed;
      }
    },

    checkMobilePortrait() {
      const isMobile = window.matchMedia("(max-width: 768px)").matches;
      const isPortrait = window.matchMedia("(orientation: portrait)").matches;
      const wasMobilePortrait = this.isMobilePortrait;

      this.isMobilePortrait = isMobile && isPortrait;

      if (this.isMobilePortrait && !wasMobilePortrait) {
        this.sidebarCollapsed = true;
        window.dispatchEvent(new CustomEvent('mobile-portrait-mode', {
          detail: { isMobilePortrait: true }
        }));
      } else if (!this.isMobilePortrait && wasMobilePortrait) {
        const savedState = localStorage.getItem("sidebarCollapsed");
        this.sidebarCollapsed = savedState === "true";
        window.dispatchEvent(new CustomEvent('mobile-portrait-mode', {
          detail: { isMobilePortrait: false }
        }));
      }
    }
  },
};
</script>

<template>
  <div class="full-page" :class="{ 'sidebar-collapsed': sidebarCollapsed, 'mobile-portrait': isMobilePortrait }">
    <div class="nav-side-layout" :class="{ collapsed: sidebarCollapsed, 'mobile-hidden': isMobilePortrait }">
      <router-view name="SideBarView"></router-view>
    </div>
    <div class="content" :class="{ 'sidebar-collapsed': sidebarCollapsed, 'mobile-full': isMobilePortrait }">
      <router-view name="ContentView"></router-view>
    </div>

    <MobileBottomMenu v-if="isMobilePortrait" :showFiltersModal="showMobileFilters" @open-filters="openMobileFilters"
      @navigate="onMobileNavigate" />

    <MobileFiltersModal v-if="isMobilePortrait" :visible="showMobileFilters" :currentStudyDate="currentStudyDate"
      @close="closeMobileFilters" @apply-filters="onMobileFiltersApplied" />
  </div>
</template>

<style>
#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  width: 99.9%;
}

/* ============================================
   SIDEBAR LAYOUT
   ============================================ */
.nav-side-layout {
  overflow: visible !important;
  position: fixed;
  top: 0;
  left: 0;
  width: 220px;
  height: 100%;
  background-color: var(--nav-side-bg-color);
  color: var(--nav-side-color);
  z-index: 1000;
  transition: width 0.3s cubic-bezier(0.4, 0, 0.2, 1), transform 0.3s ease;
}

.nav-side-layout.collapsed {
  width: 60px;
}

/* 🆕 На мобильном в портретном режиме полностью скрываем sidebar */
.nav-side-layout.mobile-hidden {
  width: 0 !important;
  transform: translateX(-100%);
  overflow: hidden;
}

.nav-side-layout .toggle-btn {
  display: none;
}

/* ============================================
   CONTENT LAYOUT
   ============================================ */
.content {
  margin-left: 220px;
  min-height: 100vh;
  transition: margin-left 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.content.sidebar-collapsed {
  margin-left: 60px;
}

/* 🆕 На мобильном в портретном режиме контент на всю ширину */
.content.mobile-full {
  margin-left: 0 !important;
}

/* ============================================
   RESPONSIVE ADJUSTMENTS
   ============================================ */
@media (max-width: 768px) {
  .nav-side-layout {
    width: 60px;
  }

  .nav-side-layout.collapsed {
    width: 0;
  }

  .content {
    margin-left: 60px;
  }

  .content.sidebar-collapsed {
    margin-left: 0;
  }
}

/* ============================================
   SMOOTH TRANSITIONS FOR CHILD ELEMENTS
   ============================================ */
.full-page {
  min-height: 100vh;
}

.full-page.mobile-portrait {
  /* Дополнительные стили для мобильного портретного режима при необходимости */
}

div#app {
  overflow: hidden;
}

tr.study-row-collapsed {
  transform: none !important;
}

.dmis-header {
  user-select: none;
}

/* =============================================================================
   MOBILE PORTRAIT STYLES
   ============================================================================= */

@media (max-width: 768px) and (orientation: portrait) {

  /* App container - offset for collapsed sidebar */
  div#app {
    margin-left: 0 !important;
  }

  /* Sidebar completely hidden */
  .nav-side-layout.mobile-hidden {
    width: 0 !important;
    transform: translateX(-100%);
    overflow: hidden;
    visibility: hidden;
  }

  /* Content takes full width */
  .content.mobile-full {
    margin-left: 0 !important;
    width: 100vw;
  }

  /* Full page adjustments */
  .full-page.mobile-portrait {
    width: 100vw;
    overflow-x: hidden;
  }

  /* Hide sidebar toggle on mobile portrait */
  .sidebar-toggle {
    display: none !important;
  }
}

/* General mobile styles (both orientations) */
@media (max-width: 768px) {
  div#app {
    overflow-x: hidden;
  }
}

@media (max-width: 768px) and (orientation: portrait) {
  .content.mobile-full {
    padding-bottom: 70px !important;
  }

  /* Карточки не должны скрываться за меню */
  .studies-cards-container {
    padding-bottom: 80px !important;
  }
}
</style>