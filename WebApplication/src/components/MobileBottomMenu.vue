<script>
import { mapState } from "vuex";

export default {
  name: "MobileBottomMenu",
  props: {
    showFiltersModal: {
      type: Boolean,
      default: false
    }
  },
  emits: ["open-filters", "navigate"],
  data() {
    return {
      activeItem: "studies",
      isNavigating: false, // 🟡 Защита от race condition
    };
  },
  computed: {
    ...mapState({
      uiOptions: (state) => state.configuration.uiOptions,
      studiesIds: (state) => state.studies.studiesIds,
      statistics: (state) => state.studies.statistics,
      isSearching: (state) => state.studies.isSearching,
      dicomTagsFilters: (state) => state.studies.dicomTagsFilters,
      labelFilters: (state) => state.studies.labelFilters,
    }),
    studyCount() {
      return this.studiesIds?.length || 0;
    },
    totalCount() {
      return this.statistics?.CountStudies || 0;
    },
    hasActiveFilters() {
      const filters = this.dicomTagsFilters || {};

      // Проверяем dicomTagsFilters
      for (const [key, value] of Object.entries(filters)) {
        if (value === "NONE" || value === null || value === undefined) continue;
        if (typeof value === "string" && value.length > 0) return true;
        if (Array.isArray(value) && value.length > 0) return true;
      }

      // Проверяем отдельные поля, если они не в dicomTagsFilters
      if (this.$store.state.studies.filterStudyDate) return true;

      return Array.isArray(this.labelFilters) && this.labelFilters.length > 0;
    },
    // 🟢 Новая computed-переменная: отключать кнопку фильтров на страницах настроек/профиля
    isFiltersDisabled() {
      const path = this.$route.path;
      return path === "/settings" || path.startsWith("/profile");
    },
  },
  watch: {
    // 🟡 Синхронизация с текущим роутом
    $route: {
      immediate: true,
      handler(to) {
        const path = to.path;
        if (path === "/settings") {
          this.activeItem = "settings";
        } else if (path.startsWith("/profile")) {
          this.activeItem = "profile";
        } else if (path === "/" || path === "/studies") {
          this.activeItem = "studies";
        }
        // filters устанавливается через showFiltersModal prop
      },
    },
  },
  methods: {
    async goToStudies() {
      if (this.isNavigating || this.activeItem === "studies") return;
      this.isNavigating = true;
      this.activeItem = "studies";

      try {
        if (this.hasActiveFilters) {
          await this.$store.dispatch("studies/clearFilterNoReload");
          // Очищаем URL от параметров фильтров
          await this.$router.replace("/");
        } else {
          await this.$router.push("/");
        }
        this.$emit("navigate", "studies");
      } finally {
        this.isNavigating = false;
      }
    },
    openFilters() {
      // 🟢 Добавлена проверка на отключённую кнопку
      if (this.isFiltersDisabled) return;

      if (this.activeItem === "filters" && this.showFiltersModal) return;
      this.activeItem = "filters";
      this.$emit("open-filters");
    },
    async goToSettings() {
      if (this.isNavigating || this.activeItem === "settings") return;
      this.isNavigating = true;

      this.activeItem = "settings";

      try {
        await this.$router.push("/settings");
        this.$emit("navigate", "settings");
      } finally {
        this.isNavigating = false;
      }
    },
    goToProfile() {
      if (this.activeItem === "profile") return;
      this.activeItem = "profile";
      this.$emit("navigate", "profile");
      // Внешний URL - используем window.location
      window.location.href = "/profile";
    },
    logout() {
      // Logout - всегда внешний endpoint
      window.location.href = "/logout";
    },
  },
};
</script>

<template>
  <div class="mobile-bottom-menu">
    <div class="menu-container">
      <!-- Все исследования -->
      <button class="menu-item" :class="{ active: activeItem === 'studies' }" @click="goToStudies"
        :title="$t('local_studies')" :disabled="isNavigating">
        <div class="icon-wrapper">
          <i class="bi bi-folder2-open"></i>
          <span v-if="studyCount > 0" class="badge-count">{{ studyCount }}</span>
        </div>
        <span class="menu-label">{{ $t('mobile_menu.studies') || 'Все' }}</span>
      </button>

      <!-- Фильтры -->
      <button class="menu-item" :class="{
        active: activeItem === 'filters' || showFiltersModal,
        'has-filters': hasActiveFilters && !isFiltersDisabled
      }" @click="openFilters" :title="$t('mobile_menu.filters') || 'Фильтры'" :disabled="isFiltersDisabled">
        <div class="icon-wrapper">
          <i class="bi bi-funnel-fill"></i>
          <span v-if="hasActiveFilters && !isFiltersDisabled" class="filter-indicator"></span>
        </div>
        <span class="menu-label">{{ $t('mobile_menu.filters') || 'Фильтр' }}</span>
      </button>

      <!-- Настройки -->
      <button class="menu-item" :class="{ active: activeItem === 'settings' }" @click="goToSettings"
        :title="$t('settings.title')" :disabled="isNavigating">
        <div class="icon-wrapper">
          <i class="bi bi-gear-fill"></i>
        </div>
        <span class="menu-label">{{ $t('mobile_menu.settings') || 'Настр.' }}</span>
      </button>

      <!-- Профиль -->
      <button class="menu-item" :class="{ active: activeItem === 'profile' }" @click="goToProfile"
        :title="$t('sidebar.profile') || 'Профиль'" :disabled="isNavigating">
        <div class="icon-wrapper">
          <i class="bi bi-person-circle"></i>
        </div>
        <span class="menu-label">{{ $t('mobile_menu.profile') || 'Профиль' }}</span>
      </button>

      <!-- Выход -->
      <button class="menu-item logout-item" @click="logout" :title="$t('logout')">
        <div class="icon-wrapper">
          <i class="bi bi-box-arrow-right"></i>
        </div>
        <span class="menu-label">{{ $t('mobile_menu.logout') || 'Выход' }}</span>
      </button>
    </div>

    <!-- Индикатор загрузки -->
    <div v-if="isSearching" class="search-indicator">
      <div class="search-progress"></div>
    </div>
  </div>
</template>

<style scoped>
/* =============================================================================
   MOBILE BOTTOM MENU - D-MIS Design System
   ============================================================================= */

.mobile-bottom-menu {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  z-index: 9999;
  background: linear-gradient(180deg, #0f172a 0%, #1e293b 100%);
  border-top: 1px solid rgba(59, 130, 246, 0.3);
  box-shadow: 0 -4px 20px rgba(0, 0, 0, 0.3);
  padding-bottom: env(safe-area-inset-bottom, 0);
}

.menu-container {
  display: flex;
  justify-content: space-around;
  align-items: center;
  padding: 6px 4px;
  max-width: 100%;
}

/* Menu Item */
.menu-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 2px;
  padding: 8px 12px;
  background: transparent;
  border: none;
  border-radius: 12px;
  color: #94a3b8;
  cursor: pointer;
  transition: all 0.2s ease;
  min-width: 56px;
  position: relative;
}

.menu-item:disabled {
  opacity: 0.5;
  /* 🟢 Пониженная яркость */
  cursor: not-allowed;
  /* 🟢 Курсор "запрещено" */
  pointer-events: none;
  /* 🟢 Полностью отключаем события (на всякий случай) */
}

.menu-item:not(:disabled):active {
  transform: scale(0.95);
}

.menu-item:not(:disabled):hover,
.menu-item:not(:disabled):focus {
  color: #e2e8f0;
  background: rgba(59, 130, 246, 0.1);
  outline: none;
}

.menu-item.active {
  color: #3b82f6;
  background: rgba(59, 130, 246, 0.15);
}

.menu-item.active .icon-wrapper i {
  transform: scale(1.1);
}

/* Icon Wrapper */
.icon-wrapper {
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 28px;
  height: 28px;
}

.icon-wrapper i {
  font-size: 20px;
  transition: all 0.2s ease;
}

/* Badge Count */
.badge-count {
  position: absolute;
  top: -4px;
  right: -8px;
  min-width: 16px;
  height: 16px;
  padding: 0 4px;
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
  color: white;
  font-size: 9px;
  font-weight: 600;
  border-radius: 8px;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: 0 2px 4px rgba(59, 130, 246, 0.4);
}

/* Filter Indicator (dot) */
.filter-indicator {
  position: absolute;
  top: -2px;
  right: -2px;
  width: 8px;
  height: 8px;
  background: linear-gradient(135deg, #10b981 0%, #059669 100%);
  border-radius: 50%;
  box-shadow: 0 0 6px rgba(16, 185, 129, 0.6);
  animation: pulse 2s infinite;
}

@keyframes pulse {

  0%,
  100% {
    transform: scale(1);
    opacity: 1;
  }

  50% {
    transform: scale(1.2);
    opacity: 0.8;
  }
}

/* Has Active Filters */
.menu-item.has-filters {
  color: #10b981;
}

.menu-item.has-filters.active {
  color: #10b981;
  background: rgba(16, 185, 129, 0.15);
}

/* Menu Label */
.menu-label {
  font-size: 10px;
  font-weight: 500;
  letter-spacing: 0.2px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  max-width: 52px;
}

/* Logout Item */
.logout-item:hover,
.logout-item:focus {
  color: #ef4444;
  background: rgba(239, 68, 68, 0.1);
}

.logout-item:active {
  background: rgba(239, 68, 68, 0.2);
}

/* Search Indicator */
.search-indicator {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  height: 2px;
  background: rgba(59, 130, 246, 0.2);
  overflow: hidden;
}

.search-progress {
  height: 100%;
  width: 30%;
  background: linear-gradient(90deg, #3b82f6, #8b5cf6, #3b82f6);
  animation: searchProgress 1.5s ease-in-out infinite;
}

@keyframes searchProgress {
  0% {
    transform: translateX(-100%);
  }

  100% {
    transform: translateX(400%);
  }
}

/* Responsive adjustments */
@media (max-width: 360px) {
  .menu-item {
    padding: 6px 8px;
    min-width: 48px;
  }

  .menu-label {
    font-size: 9px;
    max-width: 44px;
  }

  .icon-wrapper i {
    font-size: 18px;
  }
}
</style>