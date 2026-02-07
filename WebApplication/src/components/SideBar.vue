<script>
import UploadHandler from "./UploadHandler.vue";
import JobsList from "./JobsList.vue";
import LanguagePicker from "./LanguagePicker.vue";
import { mapState, mapGetters } from "vuex";
import { orthancApiUrl, oe2ApiUrl } from "../globalConfigurations";
import api from "../orthancApi";
import SourceType from "../helpers/source-type";

export default {
  props: [],
  emits: [],
  data() {
    return {
      modalitiesEchoStatus: {},
      labelsStudyCount: {},
      noLabelsStudyCount: null,
      isAdmin: false,
      isMobilePortrait: false,
      isCollapsed: false,
    };
  },
  computed: {
    ...mapState({
      uiOptions: (state) => state.configuration.uiOptions,
      userProfile: (state) => state.configuration.userProfile,
      system: (state) => state.configuration.system,
      queryableDicomModalities: (state) =>
        state.configuration.queryableDicomModalities,
      queryableDicomWebServers: (state) =>
        state.configuration.queryableDicomWebServers,
      studiesIds: (state) => state.studies.studiesIds,
      statistics: (state) => state.studies.statistics,
      labelFilters: (state) => state.studies.labelFilters,
      labelsContraint: (state) => state.studies.labelsContraint,
      jobs: (state) => state.jobs.jobsIds,
      allLabels: (state) => state.labels.allLabels,
      canShowStudiesWithoutLabels: (state) =>
        state.labels.canShowStudiesWithoutLabels,
      hasCustomLogo: (state) => state.configuration.hasCustomLogo,
      configuration: (state) => state.configuration,
      studiesSourceType: (state) => state.studies.sourceType,
      studiesRemoteSource: (state) => state.studies.remoteSource,
      hasExtendedFind: (state) => state.configuration.hasExtendedFind,
      installedPlugins: (state) => state.configuration.installedPlugins,
    }),
    customLogoUrl() {
      if (this.hasCustomLogo && this.configuration.customLogoUrl) {
        return this.configuration.customLogoUrl;
      } else {
        return "./customizable/custom-logo";
      }
    },
    hasQueryableDicomWebServers() {
      return this.queryableDicomWebServers.length > 0;
    },
    hasQueryableDicomModalities() {
      return (
        this.uiOptions.EnableDicomModalities &&
        Object.keys(this.queryableDicomModalities).length > 0
      );
    },
    hasAccessToSettings() {
      return this.uiOptions.EnableSettings;
    },
    hasAccessToWorklists() {
      return (
        "orthanc-worklists" in this.installedPlugins &&
        this.installedPlugins["orthanc-worklists"].Enabled &&
        this.uiOptions.EnableWorklists
      );
    },
    hasAccessToSettingsLabelsAndPermissions() {
      return (
        this.hasAccessToSettings && this.uiOptions.EnablePermissionsEdition
      );
    },
    hasJobs() {
      return this.jobs.length > 0;
    },
    hasUserProfile() {
      return this.userProfile != null && this.userProfile.name;
    },
    displayedStudyCount() {
      if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
        return this.studiesIds.length;
      } else {
        return "-";
      }
    },
    orthancApiUrl() {
      return orthancApiUrl;
    },
  },
  methods: {
    handleMobilePortraitMode(e) {
      this.isMobilePortrait = e.detail.isMobilePortrait;
      if (this.isMobilePortrait) {
        this.isCollapsed = true;
        // НЕ сохраняем в localStorage - это временное состояние
      }
    },

    // 🆕 ДОБАВИТЬ: Начальная проверка
    checkMobilePortrait() {
      const isMobile = window.matchMedia("(max-width: 768px)").matches;
      const isPortrait = window.matchMedia("(orientation: portrait)").matches;
      this.isMobilePortrait = isMobile && isPortrait;

      if (this.isMobilePortrait) {
        this.isCollapsed = true;
      }
    },

    toggleSidebar() {
      // Блокируем раскрытие на мобильном в портретном режиме
      if (this.isMobilePortrait) {
        return;
      }

      this.isCollapsed = !this.isCollapsed;
      localStorage.setItem("sidebarCollapsed", this.isCollapsed);
      window.dispatchEvent(
        new CustomEvent("sidebar-collapsed", {
          detail: { collapsed: this.isCollapsed },
        }),
      );
    },
    isSelectedModality(modality) {
      return (
        this.studiesSourceType == SourceType.REMOTE_DICOM &&
        this.studiesRemoteSource == modality
      );
    },
    isSelectedDicomWebServer(server) {
      return (
        this.studiesSourceType == SourceType.REMOTE_DICOM_WEB &&
        this.studiesRemoteSource == server
      );
    },
    isEchoRunning(modality) {
      return this.modalitiesEchoStatus[modality] == null;
    },
    isEchoSuccess(modality) {
      return this.modalitiesEchoStatus[modality] == true;
    },
    async selectLabel(label) {
      this.$router.push({
        name: "local-studies-list",
        query: { labels: label },
      });
    },
    async selectWithoutLabels() {
      this.$router.push({
        name: "local-studies-list",
        query: { "without-labels": true },
      });
    },
    isSelectedLabel(label) {
      return (
        this.labelsContraint != "None" && this.labelFilters.includes(label)
      );
    },
    isSelectedWithoutLabels() {
      return this.labelsContraint == "None" && this.labelFilters.length == 0;
    },
    logout(event) {
      event.preventDefault();
      window.location.href = "/logout";
    },
    goToProfile(event) {
      event.preventDefault();
      window.location.href = "/profile";
    },
    goToDicomRouter(event) {
      event.preventDefault();
      window.location.href = "/dicom-router";
    },
    goToAdminDashboard(event) {
      event.preventDefault();
      window.location.href = "/admin/dashboard";
    },
    async checkAdminRights() {
      try {
        const response = await fetch("/api/permissions");
        if (response.ok) {
          this.isAdmin = true;
        } else {
          this.isAdmin = false;
        }
      } catch (e) {
        this.isAdmin = localStorage.getItem("isAdmin") === "true";
      }
    },
    async loadLabelsCount() {
      if (Object.entries(this.labelsStudyCount).length == 0) {
        for (const label of this.allLabels) {
          this.labelsStudyCount[label] = null;
        }
      }
      if (this.hasExtendedFind) {
        if (this.uiOptions.EnableLabelsCount) {
          for (const [k, v] of Object.entries(this.labelsStudyCount)) {
            if (v == null) {
              this.labelsStudyCount[k] = await api.getLabelStudyCount(k);
            }
          }
          if (this.canShowStudiesWithoutLabels) {
            this.noLabelsStudyCount = await api.getLabelStudyCount(null);
          }
        }
      }
    },
  },
  watch: {
    allLabels(newValue, oldValue) {
      this.loadLabelsCount();
    },
  },
  beforeUnmount() {
    window.removeEventListener("mobile-portrait-mode", this.handleMobilePortraitMode);
    window.removeEventListener("resize", this.checkMobilePortrait);
  },
  mounted() {
    // 🆕 ДОБАВИТЬ: Слушатель мобильного режима
    window.addEventListener("mobile-portrait-mode", this.handleMobilePortraitMode);

    // 🆕 ДОБАВИТЬ: Начальная проверка
    this.checkMobilePortrait();
    const savedState = localStorage.getItem("sidebarCollapsed");
    if (savedState !== null) {
      this.isCollapsed = savedState === "true";
      window.dispatchEvent(
        new CustomEvent("sidebar-collapsed", {
          detail: { collapsed: this.isCollapsed },
        }),
      );
    }

    this.checkAdminRights();
    this.loadLabelsCount();
    this.$refs["modalities-collapsible"]?.addEventListener(
      "show.bs.collapse",
      (e) => {
        for (const modality of Object.keys(this.queryableDicomModalities)) {
          this.modalitiesEchoStatus[modality] = null;
        }
        for (const [modality, config] of Object.entries(
          this.queryableDicomModalities,
        )) {
          api
            .remoteModalityEcho(modality)
            .then((response) => {
              this.modalitiesEchoStatus[modality] = true;
            })
            .catch(() => {
              this.modalitiesEchoStatus[modality] = false;
            });
        }
      },
    );
  },
  components: { UploadHandler, JobsList, LanguagePicker },
};
</script>

<template>

  <div class="nav-side-menu" :class="{ collapsed: isCollapsed }">
    <div class="nav-side-content">
      <!-- Header with Logo -->
      <div class="dmis-header">
        <svg viewBox="0 0 100 100" class="dmis-logo-svg">
          <defs>
            <linearGradient id="dmisGrad" x1="0%" y1="0%" x2="100%" y2="100%">
              <stop offset="0%" style="stop-color: #3b82f6; stop-opacity: 1" />
              <stop offset="100%" style="stop-color: #8b5cf6; stop-opacity: 1" />
            </linearGradient>
          </defs>
          <circle cx="50" cy="50" r="45" fill="url(#dmisGrad)" />
          <text x="50" y="68" text-anchor="middle" fill="white" font-size="50" font-weight="bold"
            font-family="Arial, sans-serif">
            D
          </text>
        </svg>
        <div v-if="!isCollapsed" class="dmis-title">D-MIS</div>
        <div v-if="!isCollapsed" class="dmis-subtitle">PACS</div>
      </div>

      <!-- Toggle Button -->
      <div class="sidebar-toggle" @click="toggleSidebar"
        :title="isCollapsed ? $t('sidebar.open') : $t('sidebar.close')">
        <i class="fa" :class="isCollapsed ? 'fa-chevron-right' : 'fa-chevron-left'"></i>
      </div>


      <div v-if="uiOptions.ShowOrthancName && !isCollapsed" class="orthanc-name">
        <p>{{ system.Name }}</p>
      </div>

      <!-- ==================== EXPANDED MENU ==================== -->
      <div v-if="!isCollapsed" class="menu-list">
        <ul id="menu-content" class="menu-content collapse out">
          <!-- Local Studies -->
          <li>
            <router-link class="router-link" to="/">
              <i class="fa fa-folder-open fa-lg menu-icon"></i>{{ $t("local_studies") }}
              <span class="study-count ms-auto">{{ displayedStudyCount }} / {{ statistics.CountStudies }}</span>
            </router-link>
          </li>

          <!-- Labels submenu -->
          <ul v-if="allLabels.length > 0" class="sub-menu" id="labels-list">
            <li v-if="canShowStudiesWithoutLabels" key="without-label" :class="{ active: isSelectedWithoutLabels() }"
              @click="selectWithoutLabels()">
              {{ $t("labels.studies_without_labels") }}
              <span class="study-count ms-auto">{{ noLabelsStudyCount }}</span>
            </li>
            <li v-for="label in allLabels" :key="label" :class="['tagLabel ', { active: isSelectedLabel(label) }]"
              @click="selectLabel(label)">
              <i class="fa fa-tag label-icon"></i><span class="cut-text">{{ label }}</span>
              <span class="study-count ms-auto">{{
                labelsStudyCount[label]
              }}</span>
            </li>
          </ul>

          <!-- Upload -->
          <li v-if="uiOptions.EnableUpload" class="d-flex align-items-center" data-bs-toggle="collapse"
            data-bs-target="#upload-handler">
            <i class="fa fa-cloud-arrow-up fa-lg menu-icon"></i>{{ $t("upload") }}
            <span class="ms-auto"></span>
          </li>
          <div v-if="uiOptions.EnableUpload" class="collapse" id="upload-handler">
            <UploadHandler :showStudyDetails="true" />
          </div>

          <!-- DICOM Modalities -->
          <li v-if="hasQueryableDicomModalities" class="d-flex align-items-center" data-bs-toggle="collapse"
            data-bs-target="#modalities-list">
            <i class="fa fa-server fa-lg menu-icon"></i>{{ $t("dicom_modalities") }}
            <span class="arrow ms-auto"></span>
          </li>
          <ul class="sub-menu collapse" id="modalities-list" ref="modalities-collapsible">
            <li v-for="modality of Object.keys(queryableDicomModalities)" :key="modality"
              :class="['mod-btn', { active: isSelectedModality(modality) }]">
              <router-link class="router-link dicom-modal" :to="{
                path: '/filtered-studies',
                query: { 'source-type': 'dicom', 'remote-source': modality },
              }"><i class="fa-solid fa-radiation mr-1"> </i>
                {{ modality }}
              </router-link>
              <span v-if="isEchoRunning(modality)" class="ms-auto spinner-border spinner-border-sm"
                data-bs-toggle="tooltip" :title="$t('checking_connectivity')"></span>
              <span v-else-if="isEchoSuccess(modality)" class="ms-auto"><i class="bi bi-check2 text-success echo-status"
                  data-bs-toggle="tooltip" :title="$t('c_echo_succeeded')"></i></span>
              <span v-else class="ms-auto"><i class="bi bi-x-lg text-danger echo-status" data-bs-toggle="tooltip"
                  :title="$t('c_echo_failed')"></i></span>
            </li>
          </ul>

          <!-- DicomWeb Servers -->
          <li v-if="hasQueryableDicomWebServers" class="d-flex align-items-center" data-bs-toggle="collapse"
            data-bs-target="#dicomweb-servers-list">
            <i class="fa fa-globe fa-lg menu-icon"></i>{{ $t("dicom_web_servers") }}
            <span class="arrow ms-auto"></span>
          </li>
          <ul class="sub-menu collapse" id="dicomweb-servers-list">
            <li v-for="server in queryableDicomWebServers" :key="server"
              :class="{ active: isSelectedDicomWebServer(server) }">
              <router-link class="router-link" :to="{
                path: '/filtered-studies',
                query: {
                  'source-type': 'dicom-web',
                  'remote-source': server,
                },
              }">
                {{ server }}
              </router-link>
            </li>
          </ul>

          <!-- External Viewers -->
          <li v-if="uiOptions.EnableOpenInMedDreamViewer" class="d-flex align-items-center">
            <a :href="'../meddream/index.html'" target="_meddream">
              <i class="fa fa-x-ray fa-lg menu-icon"></i>{{ $t("open_meddream") }}
            </a>
            <span class="ms-auto"></span>
          </li>
          <li v-if="uiOptions.EnableOpenInStoneViewer" class="d-flex align-items-center">
            <a :href="'../stone-webviewer/index.html'" target="_stone">
              <i class="fa fa-cube fa-lg menu-icon"></i>{{ $t("open_stone_viewer") }}
            </a>
            <span class="ms-auto"></span>
          </li>
          <li v-if="uiOptions.EnableOpenInVolView" class="d-flex align-items-center">
            <a :href="'../volview/'" target="_volview">
              <i class="fa fa-box fa-lg menu-icon"></i>{{ $t("open_volview") }}
            </a>
            <span class="ms-auto"></span>
          </li>

          <!-- Worklists -->
          <li v-if="hasAccessToWorklists">
            <router-link class="router-link" to="/worklists">
              <i class="fa fa-list fa-lg menu-icon"></i>{{ $t("worklists.side_bar_title") }}
            </router-link>
          </li>

          <!-- Settings -->
          <li v-if="hasAccessToSettings" class="d-flex align-items-center" data-bs-toggle="collapse"
            data-bs-target="#settings-list">
            <i class="fa fa-sliders fa-lg menu-icon"></i>{{ $t("settings.title") }}
            <span class="arrow ms-auto"></span>
          </li>
          <ul class="sub-menu collapse" id="settings-list">
            <li>
              <router-link class="router-link" to="/settings">
                <i class="fa fa-circle-info menu-icon"></i>{{ $t("settings.system_info") }}
              </router-link>
            </li>
            <li v-if="hasAccessToSettingsLabelsAndPermissions">
              <router-link class="router-link" to="/settings-labels">
                <i class="fa fa-tags menu-icon"></i>{{ $t("settings.available_labels_title") }}
              </router-link>
            </li>
            <li v-if="hasAccessToSettingsLabelsAndPermissions">
              <router-link class="router-link" to="/settings-permissions">
                <i class="fa fa-user-shield menu-icon"></i>{{ $t("settings.permissions") }}
              </router-link>
            </li>
            <li>
              <a href="#" @click="goToProfile($event)">
                <i class="fa fa-user-circle menu-icon"></i>{{ $t("sidebar.profile") }}
              </a>
            </li>
            <li v-if="isAdmin">
              <a href="#" @click="goToAdminDashboard($event)">
                <i class="fa fa-chart-line menu-icon"></i>{{ $t("sidebar.dashboard") }}
              </a>
            </li>
            <li v-if="isAdmin">
              <a href="#" @click="goToDicomRouter($event)">
                <i class="fa fa-network-wired menu-icon"></i>DICOM Router
              </a>
            </li>
            <li class="logout-item">
              <a href="#" @click="logout($event)">
                <i class="fa fa-power-off menu-icon"></i>{{ $t("logout") }}
              </a>
            </li>
          </ul>

          <!-- Audit Logs -->
          <li v-if="uiOptions.EnableAuditLogs">
            <router-link class="router-link" to="/audit-logs">
              <i class="fa fa-table-list menu-icon"></i>{{ $t("audit_logs.side_bar_title") }}
            </router-link>
          </li>

          <!-- Legacy UI -->
          <li v-if="uiOptions.EnableLinkToLegacyUi">
            <a :href="orthancApiUrl + 'app/explorer.html'">
              <i class="fa fa-solid fa-backward fa-lg menu-icon"></i>{{ $t("legacy_ui") }}
            </a>
            <span class="ms-auto"></span>
          </li>

          <!-- Jobs -->
          <li v-if="hasJobs" class="d-flex align-items-center">
            <a href="#">
              <i class="fa fa-solid fa-bars-progress fa-lg menu-icon"></i>{{ $t("my_jobs") }}
            </a>
            <span class="ms-auto"></span>
          </li>
          <div v-if="hasJobs" class="collapse show" id="jobs-list">
            <JobsList />
          </div>
        </ul>
      </div>

      <!-- ==================== COLLAPSED MENU (only 3 items) ==================== -->
      <div v-if="isCollapsed" class="menu-list collapsed-menu">
        <ul class="menu-content">
          <!-- Local Studies -->
          <li class="collapsed-item" :title="$t('local_studies')">
            <router-link class="router-link" to="/">
              <i class="fa fa-folder-open"></i>
            </router-link>
          </li>

          <!-- Settings -->
          <li v-if="hasAccessToSettings" class="collapsed-item" :title="$t('settings.title')">
            <router-link class="router-link" to="/settings">
              <i class="fa fa-sliders"></i>
            </router-link>
          </li>

          <!-- Logout -->
          <li class="collapsed-item logout-collapsed" :title="$t('logout')">
            <a href="#" @click="logout($event)">
              <i class="fa fa-power-off"></i>
            </a>
          </li>
        </ul>
      </div>

      <!-- Language Picker (only expanded) -->
      <div class="bottom-side-bar">
        <div class="bottom-side-bar-button">
          <LanguagePicker :isCollapsed="isCollapsed" />
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
/* =============================================================================
   SIDEBAR TOGGLE BUTTON
   ============================================================================= */
.sidebar-toggle {
  position: absolute;
  bottom: -28px;
  right: 0px;
  transform: translateY(-50%);
  width: 28px;
  height: 56px;
  color: #94a3b8;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 12px;
  z-index: 9999;
  transition: all 0.3s ease;
  box-shadow: 4px 0 12px rgba(0, 0, 0, 0.2);
}

.sidebar-toggle:hover {
  background: linear-gradient(135deg, #3b82f6 0%, #8b5cf6 100%);
  color: white;
  width: 32px;
}

/* Ensure toggle is always visible and clickable */
.nav-side-menu.collapsed .sidebar-toggle {
  right: 0;
  z-index: 9999;
}

/* =============================================================================
   D-MIS HEADER
   ============================================================================= */
.dmis-header {
  text-align: center;
  padding: 20px 10px 15px;
  margin-bottom: 10px;
  background: linear-gradient(180deg,
      rgba(59, 130, 246, 0.1) 0%,
      transparent 100%);
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
  transition: all 0.3s ease;
}

.dmis-logo-svg {
  width: 70px;
  height: 70px;
  margin: 0 auto;
  display: block;
  filter: drop-shadow(0 4px 6px rgba(0, 0, 0, 0.3));
  transition: all 0.3s ease;
}

.dmis-title {
  font-size: 22px;
  font-weight: 700;
  color: #e2e8f0;
  margin-top: 8px;
  letter-spacing: 2px;
}

.dmis-subtitle {
  font-size: 11px;
  color: #64748b;
  font-weight: 500;
  text-transform: uppercase;
  letter-spacing: 3px;
  margin-top: 2px;
}

/* =============================================================================
   COLLAPSED STATE - Header
   ============================================================================= */
.nav-side-menu.collapsed {
  width: 60px !important;
}

.nav-side-menu.collapsed .dmis-header {
  padding: 15px 5px 12px;
}

.nav-side-menu.collapsed .dmis-logo-svg {
  width: 40px;
  height: 40px;
}

/* =============================================================================
   COLLAPSED MENU ITEMS
   ============================================================================= */
.collapsed-menu {
  margin: 0 !important;
  padding: 10px 0;
}

.collapsed-menu .menu-content {
  display: flex;
  flex-direction: column;
  gap: 4px;
  padding: 0;
  margin: 0;
}

.collapsed-item {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 48px;
  list-style: none;
  margin: 0;
  padding: 0;
  border-left: 3px solid transparent;
  transition: all 0.2s ease;
  position: relative;
}

.collapsed-item a,
.collapsed-item .router-link {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 100%;
  height: 100%;
  color: #94a3b8;
  text-decoration: none;
  transition: all 0.2s ease;
}

.collapsed-item i {
  font-size: 20px;
  transition: all 0.2s ease;
}

.collapsed-item:hover {
  background: rgba(59, 130, 246, 0.1);
  border-left-color: #3b82f6;
}

.collapsed-item:hover a,
.collapsed-item:hover .router-link {
  color: #e2e8f0;
}

.collapsed-item:hover i {
  transform: scale(1.15);
}

/* Logout special styling */
.logout-collapsed:hover {
  background: rgba(239, 68, 68, 0.1);
  border-left-color: #ef4444;
}

.logout-collapsed:hover a {
  color: #ef4444;
}

/* Tooltip for collapsed items */
.collapsed-item::after {
  content: attr(title);
  position: absolute;
  left: 65px;
  top: 50%;
  transform: translateY(-50%);
  background: #1e293b;
  color: #e2e8f0;
  padding: 8px 14px;
  border-radius: 6px;
  font-size: 13px;
  font-weight: 500;
  white-space: nowrap;
  opacity: 0;
  visibility: hidden;
  transition: all 0.2s ease;
  z-index: 9999;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.4);
  border: 1px solid rgba(59, 130, 246, 0.2);
  pointer-events: none;
}

.collapsed-item::before {
  content: "";
  position: absolute;
  left: 57px;
  top: 50%;
  transform: translateY(-50%);
  border: 6px solid transparent;
  border-right-color: #1e293b;
  opacity: 0;
  visibility: hidden;
  transition: all 0.2s ease;
  z-index: 9999;
  pointer-events: none;
}

.collapsed-item:hover::after,
.collapsed-item:hover::before {
  opacity: 1;
  visibility: visible;
}

/* =============================================================================
   BASE STYLES (for expanded state)
   ============================================================================= */
.nav-side-menu {
  font-family:
    "Segoe UI",
    system-ui,
    -apple-system,
    sans-serif;
  font-size: 13px;
  font-weight: 400;
  background-color: #0f172a;
  color: #94a3b8;
  position: relative;
  transition: width 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  width: 220px;
  overflow: visible !important;
}

.nav-side-content {
  display: flex;
  flex-direction: column;
  min-height: 100vh;
  overflow-x: hidden;
  overflow-y: auto;
}

.router-link {
  width: 100%;
  text-align: left;
}

.echo-status {
  font-size: 17px;
}

.menu-icon {
  width: 24px;
  margin-right: 12px;
  text-align: center;
  transition: all 0.2s ease;
  flex-shrink: 0;
}

.nav-side-menu li:hover .menu-icon {
  transform: scale(1.1);
}

.logout-item:hover .menu-icon {
  color: #ef4444 !important;
}

.orthanc-name {
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
  font-size: 1rem;
  margin-bottom: 0.3rem;
  padding: 0 10px;
}

.orthanc-name p {
  margin-bottom: 0.3rem;
  font-weight: 500;
}

.bottom-side-bar {
  flex: 1;
  align-self: flex-end;
  width: 100%;
  position: relative;
  min-height: 5rem;
}

.bottom-side-bar-button {
  position: absolute;
  bottom: 0;
  width: 100%;
  height: 3rem;
}

.nav-side-menu ul,
.nav-side-menu li {
  list-style: none;
  padding: 0px;
  margin: 0px;
  line-height: 40px;
  cursor: pointer;
  transition: all 0.2s ease;
}

.nav-side-menu ul :not(collapsed) .arrow:before,
.nav-side-menu li :not(collapsed) .arrow:before {
  font-family: "Font Awesome 6 Free";
  font-weight: 900;
  content: "\f0d7";
  display: inline-block;
  padding-left: 10px;
  padding-right: 10px;
  vertical-align: middle;
  float: right;
  transition: transform 0.3s;
}

.nav-side-menu li.active .arrow:before {
  transform: rotate(180deg);
}

.nav-side-menu li .study-count {
  padding-right: 4px;
  float: right;
  font-size: 12px;
  color: #64748b;
}

.nav-side-menu ul .active,
.nav-side-menu li .active {
  border-left: 3px solid #3b82f6;
  background-color: rgba(59, 130, 246, 0.1);
  color: #e2e8f0;
}

.nav-side-menu ul .sub-menu li.active,
.nav-side-menu li .sub-menu li.active {
  border-left: 3px solid #8b5cf6;
  background-color: rgba(139, 92, 246, 0.1);
  color: #e2e8f0;
  margin-left: 4px;
}

li.mod-btn {
  margin-left: 8px;
}

.nav-side-menu ul .sub-menu li.active a,
.nav-side-menu li .sub-menu li.active a {
  color: #e2e8f0;
}

.nav-side-menu ul .sub-menu li,
.nav-side-menu li .sub-menu li {
  display: flex;
  background-color: rgba(30, 41, 59, 0.5);
  border: none;
  line-height: 36px;
  border-bottom: 2px solid rgba(255, 255, 255, 0.05);

  font-size: 12px;
}

.nav-side-menu ul .sub-menu li:hover,
.nav-side-menu li .sub-menu li:hover {
  border-left: 3px solid #8b5cf6;
  background-color: rgba(139, 92, 246, 0.1);
  color: #e2e8f0;
}

.nav-side-menu li {

  padding-left: 0px;
  border-left: 3px solid transparent;

  border-radius: 0 8px 8px 0;

}

.nav-side-menu li a {
  text-decoration: none;
  color: #94a3b8;
  display: flex;
  align-items: center;
  width: 100%;
  transition: all 0.2s;
  font-weight: 100;
  padding: 0;
  margin-left: 0;
}

.nav-side-menu li a:hover {
  color: #e2e8f0;
}

.nav-side-menu li:hover {
  border-left: 3px solid #3b82f6;
  background-color: rgba(59, 130, 246, 0.05);
  color: #e2e8f0;
}

.nav-side-menu .menu-list .menu-content {
  display: block;
}

.menu-list {
  margin-left: 2px;
  margin-right: 8px;
  font-size: 12px;
}



.label-icon {
  width: 16px;
  margin-right: 8px;
  line-height: 28px;
  opacity: 0.7;
}

li.tagLabel {
  padding: 0 6px !important;
  max-width: 110px;
  width: auto;
  overflow: hidden;

}

ul#labels-list {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(98px, 0fr));
  gap: 2px 4px;
  animation: fadeIn .2s ease;
  justify-items: stretch;
  justify-content: space-around;
}

.collapsed .sidebar-toggle {
  width: 64px !important;
  ;
}

.collapsed .sidebar-toggle:hover {
  width: 64px !important;
}
</style>
