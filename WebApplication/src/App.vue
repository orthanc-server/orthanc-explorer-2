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

// 🆕 Утилита для определения мобильного портрета (единая логика)
const checkMobilePortrait = () => {
  return window.matchMedia("(max-width: 768px)").matches &&
    window.matchMedia("(orientation: portrait)").matches;
};

export default {
  components: {
    MobileBottomMenu,
    MobileFiltersModal,
  },
  data() {
    // 🆕 Синхронная проверка при инициализации (window уже доступен в data при клиентском рендере)
    const isMobilePortrait = typeof window !== 'undefined' ? checkMobilePortrait() : false;

    return {
      sidebarCollapsed: isMobilePortrait ? true : (localStorage.getItem("sidebarCollapsed") === "true"),
      isMobilePortrait: isMobilePortrait,
      showMobileFilters: false,
      isHydrated: false, // 🆕 Флаг полной гидратации
    };
  },
  computed: {
    currentStudyDate() {
      return this.$store.state.studies.dicomTagsFilters?.StudyDate || "";
    },
    // 🆕 Вычисляемое свойство для классов контента
    contentClasses() {
      return {
        'sidebar-collapsed': this.sidebarCollapsed && !this.isMobilePortrait,
        'mobile-full': this.isMobilePortrait,
        'no-transition': !this.isHydrated, // 🆕 Отключаем transition до гидратации
      };
    },
    sidebarClasses() {
      return {
        'collapsed': this.sidebarCollapsed && !this.isMobilePortrait,
        'mobile-hidden': this.isMobilePortrait,
        'no-transition': !this.isHydrated, // 🆕 Отключаем transition до гидратации
      };
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

    // 🆕 Проверяем ещё раз после загрузки (на случай изменения ориентации во время загрузки)
    this.isMobilePortrait = checkMobilePortrait();

    window.addEventListener("resize", this.handleResize);
    window.addEventListener("orientationchange", this.handleOrientationChange);
    window.addEventListener("sidebar-collapsed", this.handleSidebarToggle);
  },
  mounted() {
    // 🆕 Включаем transitions только после полного монтирования и рендера
    requestAnimationFrame(() => {
      requestAnimationFrame(() => {
        this.isHydrated = true;
        // 🆕 Убираем preload классы из HTML
        document.documentElement.classList.remove('preload', 'mobile-portrait-initial');
      });
    });
  },
  beforeUnmount() {
    window.removeEventListener("sidebar-collapsed", this.handleSidebarToggle);
    window.removeEventListener("resize", this.handleResize);
    window.removeEventListener("orientationchange", this.handleOrientationChange);
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
    // 🆕 Объединяем логику resize/orientationchange
    handleResize() {
      this.updateMobileState();
    },
    handleOrientationChange() {
      // Небольшая задержка для корректного определения после поворота
      setTimeout(() => this.updateMobileState(), 100);
    },
    updateMobileState() {
      const wasMobilePortrait = this.isMobilePortrait;
      this.isMobilePortrait = checkMobilePortrait();

      if (this.isMobilePortrait && !wasMobilePortrait) {
        // Переход на мобильный портрет
        this.sidebarCollapsed = true;
        window.dispatchEvent(new CustomEvent('mobile-portrait-mode', {
          detail: { isMobilePortrait: true }
        }));
      } else if (!this.isMobilePortrait && wasMobilePortrait) {
        // Выход из мобильного портрета
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
  <div class="full-page" :class="{
    'sidebar-collapsed': sidebarCollapsed && !isMobilePortrait,
    'mobile-portrait': isMobilePortrait,
    'layout-ready': isHydrated  // 🆕 класс для включения анимаций
  }">
    <!-- Sidebar: v-if полностью убирает из DOM на мобильных -->
    <div v-if="!isMobilePortrait" class="nav-side-layout" :class="sidebarClasses">
      <router-view name="SideBarView"></router-view>
    </div>

    <!-- Content: вычисляемые классы -->
    <div class="content" :class="contentClasses">
      <router-view name="ContentView"></router-view>
    </div>

    <!-- Мобильное меню -->
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
  /* 🆕 Transition только когда layout готов */
  transition: none;
  /* По умолчанию без анимации */
}

/* 🆕 Включаем анимацию только после гидратации */
.layout-ready .nav-side-layout {
  transition: width 0.3s cubic-bezier(0.4, 0, 0.2, 1), transform 0.3s ease;
}

.nav-side-layout.collapsed {
  width: 60px;
}

/* 🆕 Явное скрытие через display:none вместо width:0 */
.nav-side-layout.mobile-hidden {
  display: none !important;
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
  /* 🆕 По умолчанию без анимации */
  transition: none;
}

/* 🆕 Включаем анимацию только после гидратации */
.layout-ready .content {
  transition: margin-left 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.content.sidebar-collapsed {
  margin-left: 60px;
}

/* 🆕 Мобильный портрет — немедленное применение без анимации */
.content.mobile-full {
  margin-left: 0 !important;
  /* Явно отключаем transition для мобильных */
  transition: none !important;
}

/* ============================================
   RESPONSIVE ADJUSTMENTS (Desktop only)
   ============================================ */
@media (min-width: 769px) {
  .nav-side-layout.collapsed {
    width: 60px;
  }

  .content.sidebar-collapsed {
    margin-left: 60px;
  }
}

/* ============================================
   MOBILE ADJUSTMENTS
   ============================================ */
@media (max-width: 768px) {

  /* На мобильных в landscape — узкий sidebar */
  .nav-side-layout {
    width: 60px;
  }

  .content {
    margin-left: 60px;
  }

  /* В портретном режиме — полное скрытие */
  .nav-side-layout.mobile-hidden {
    display: none !important;
  }

  .content.mobile-full {
    margin-left: 0 !important;
    width: 100vw;
    transition: none !important;
  }
}

@media (max-width: 768px) and (orientation: portrait) {
  div#app {
    margin-left: 0 !important;
  }

  .full-page.mobile-portrait {
    width: 100vw;
    overflow-x: hidden;
  }

  .content.mobile-full {
    padding-bottom: 70px !important;
  }

  .studies-cards-container {
    padding-bottom: 80px !important;
  }

  .sidebar-toggle {
    display: none !important;
  }
}

/* ============================================
   UTILITY CLASSES
   ============================================ */
.full-page {
  min-height: 100vh;
}

/* 🆕 Отключение transition */
.no-transition,
.no-transition * {
  transition: none !important;
  animation: none !important;
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
</style>