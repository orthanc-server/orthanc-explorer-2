<script>
import { mapState } from "vuex"
import api from "../orthancApi"
import LanguagePicker from "./LanguagePicker.vue"
import Devices from "./devices.vue"

export default {
    components: {
        LanguagePicker,
        Devices
    },

    props: [],
    emits: [],

    data() {
        return {
            verboseLevel: "default",
            delayedDeletionPendingFilesCount: 0,
            advancedStoragePendingDeletionFilesCount: 0,
            advancedStorageIndexerModeEnabled: false,
            advancedStorageDelayedDeletionModeEnabled: false,
            hkLastProcessedChange: -1,
            hkLastChangeToProcess: -1,
            isAdmin: false,
        };
    },

    async mounted() {
        this.verboseLevel = await api.getVerboseLevel();
        await this.loadPluginsStatus();
        await this.checkAdminRights();
    },

    methods: {
        async setVerboseLevel(level) {
            this.verboseLevel = level;
            await api.setVerboseLevel(level);
        },

        async loadPluginsStatus() {
            if (this.hasDelayedDeletionPlugin) {
                const ddStatus = await api.getDelayedDeletionStatus();
                this.delayedDeletionPendingFilesCount = ddStatus["FilesPendingDeletion"];
            }
            if (this.hasHousekeeperPlugin) {
                const hkStatus = await api.getHousekeeperStatus();
                this.hkLastChangeToProcess = hkStatus["LastChangeToProcess"];
                this.hkLastProcessedChange = hkStatus["LastProcessedChange"];
            }
            if (this.hasAdvancedStorage) {
                const advstStatus = await api.getAdvancedStorageStatus();
                this.advancedStorageDelayedDeletionModeEnabled = advstStatus["DelayedDeletionIsActive"];
                this.advancedStorageIndexerModeEnabled = advstStatus["IndexerIsActive"];
                this.advancedStoragePendingDeletionFilesCount = advstStatus["FilesPendingDeletion"];
            }
        },

        async checkAdminRights() {
            try {
                const response = await fetch("/api/permissions");
                this.isAdmin = response.ok;
            } catch (e) {
                this.isAdmin = localStorage.getItem("isAdmin") === "true";
            }
        },

        goToDicomRouter() {
            window.location.href = "/dicom-router";
        },

        goToAdminDashboard() {
            window.location.href = "/admin/dashboard";
        }
    },

    watch: {
        async isConfigurationLoaded(newValue, oldValue) {
            this.loadPluginsStatus();
        }
    },

    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            isConfigurationLoaded: state => state.configuration.loaded,
            tokens: state => state.configuration.tokens,
            statistics: state => state.studies.statistics,
            system: state => state.configuration.system,
            installedPlugins: state => state.configuration.installedPlugins,
        }),

        totalDiskSize() {
            if (this.statistics.TotalDiskSizeMB > 1024 * 1024) {
                return (Math.round(this.statistics.TotalDiskSizeMB / (1024 * 1024) * 100) / 100) + " TB";
            }
            else if (this.statistics.TotalDiskSizeMB > 1024) {
                return (Math.round(this.statistics.TotalDiskSizeMB / 1024 * 100) / 100) + " GB";
            }
            return this.statistics.TotalDiskSizeMB + " MB";
        },

        hasMaxPatientCount() {
            return this.system.MaximumPatientCount > 0;
        },

        hasMaxStorageSize() {
            return this.system.MaximumStorageSize > 0;
        },

        maxStorageSize() {
            if (this.system.MaximumStorageSize > 1024 * 1024) {
                return (Math.round(this.system.MaximumStorageSize / (1024 * 1024) * 100) / 100) + " TB";
            }
            else if (this.statistics.TotalDiskSizeMB > 1024) {
                return (Math.round(this.system.MaximumStorageSize / 1024 * 100) / 100) + " GB";
            }
            return this.system.MaximumStorageSize + " MB";
        },

        hasDelayedDeletionPlugin() {
            return 'delayed-deletion' in this.installedPlugins && this.installedPlugins['delayed-deletion'].Enabled;
        },

        async hasAdvancedStorageDelayedDeletion() {
            return 'advanced-storage' in this.installedPlugins && this.installedPlugins['advanced-storage'].Enabled && this.advancedStorageDelayedDeletionModeEnabled;
        },

        hasHousekeeperPlugin() {
            return 'housekeeper' in this.installedPlugins && this.installedPlugins['housekeeper'].Enabled;
        },

        hasAdvancedStorage() {
            return 'advanced-storage' in this.installedPlugins && this.installedPlugins['advanced-storage'].Enabled;
        },

        hkPctDone() {
            if (this.hkLastProcessedChange >= 1) {
                return 100 * (this.hkLastProcessedChange / this.hkLastChangeToProcess);
            } else {
                return 100;
            }
        },

        hkPctRemaining() {
            return 100 - this.hkPctDone;
        },

        hkStatusText() {
            if (this.hkLastProcessedChange >= 1) {
                return this.hkLastProcessedChange + " / " + this.hkLastChangeToProcess;
            } else {
                return this.$t('plugins.housekeeper.completed')
            }
        }
    },
}
</script>

<template>
    <div class="settings-container">
        <!-- Header -->
        <div class="settings-header">
            <div class="header-left">
                <div class="header-title">
                    <i class="bi bi-gear-fill"></i>
                    <h1>{{ $t('settings.title') }}</h1>
                </div>

                <!-- Admin Navigation -->
                <div v-if="isAdmin" class="admin-nav">
                    <button class="nav-btn" @click="goToDicomRouter">
                        <i class="bi bi-router-fill"></i>
                        <span>DICOM Router</span>
                    </button>
                    <button class="nav-btn" @click="goToAdminDashboard">
                        <i class="bi bi-speedometer2"></i>
                        <span>Dashboard</span>
                    </button>
                </div>
            </div>

            <div class="header-actions">
                <LanguagePicker :isCollapsed="false" />
            </div>
        </div>

        <div class="settings-grid">
            <!-- Statistics Card -->
            <div class="settings-card">
                <div class="card-header">
                    <i class="bi bi-bar-chart-line-fill"></i>
                    <h3>{{ $t('settings.statistics') }}</h3>
                </div>
                <div class="card-body">
                    <div class="stat-grid">
                        <div class="stat-item">
                            <div class="stat-icon patients">
                                <i class="bi bi-people-fill"></i>
                            </div>
                            <div class="stat-info">
                                <span class="stat-label">{{ $t('patients') }}</span>
                                <span class="stat-value" v-if="!hasMaxPatientCount">{{ statistics.CountPatients
                                }}</span>
                                <span class="stat-value" v-else>{{ statistics.CountPatients }} / {{
                                    system.MaximumPatientCount }}</span>
                            </div>
                        </div>

                        <div class="stat-item">
                            <div class="stat-icon studies">
                                <i class="bi bi-folder-fill"></i>
                            </div>
                            <div class="stat-info">
                                <span class="stat-label">{{ $t('studies') }}</span>
                                <span class="stat-value">{{ statistics.CountStudies }}</span>
                            </div>
                        </div>

                        <div class="stat-item">
                            <div class="stat-icon series">
                                <i class="bi bi-layers-fill"></i>
                            </div>
                            <div class="stat-info">
                                <span class="stat-label">{{ $t('series_plural') }}</span>
                                <span class="stat-value">{{ statistics.CountSeries }}</span>
                            </div>
                        </div>

                        <div class="stat-item">
                            <div class="stat-icon instances">
                                <i class="bi bi-file-earmark-image-fill"></i>
                            </div>
                            <div class="stat-info">
                                <span class="stat-label">{{ $t('instances') }}</span>
                                <span class="stat-value">{{ statistics.CountInstances }}</span>
                            </div>
                        </div>

                        <div class="stat-item wide">
                            <div class="stat-icon storage">
                                <i class="bi bi-hdd-fill"></i>
                            </div>
                            <div class="stat-info">
                                <span class="stat-label">{{ $t('settings.storage_size') }}</span>
                                <span class="stat-value" v-if="!hasMaxStorageSize">{{ totalDiskSize }}</span>
                                <span class="stat-value" v-else>{{ totalDiskSize }} / {{ maxStorageSize }}</span>
                            </div>
                            <div class="storage-bar" v-if="hasMaxStorageSize">
                                <div class="progress">
                                    <div class="progress-bar"
                                        :style="{ width: (statistics.TotalDiskSizeMB / system.MaximumStorageSize * 100) + '%' }">
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>

                    <!-- Plugin Stats -->
                    <div class="plugin-stats"
                        v-if="hasDelayedDeletionPlugin || hasAdvancedStorageDelayedDeletion || hasHousekeeperPlugin">
                        <div class="plugin-stat-item" v-if="hasDelayedDeletionPlugin">
                            <i class="bi bi-clock-history"></i>
                            <span>{{ $t('plugins.delayed_deletion.pending_files_count') }}</span>
                            <strong>{{ delayedDeletionPendingFilesCount }}</strong>
                        </div>

                        <div class="plugin-stat-item" v-if="hasAdvancedStorageDelayedDeletion">
                            <i class="bi bi-trash-fill"></i>
                            <span>{{ $t('plugins.advanced_storage.pending_deletion_files_count') }}</span>
                            <strong>{{ advancedStoragePendingDeletionFilesCount }}</strong>
                        </div>
                    </div>

                    <!-- Housekeeper Progress -->
                    <div class="housekeeper-progress" v-if="hasHousekeeperPlugin">
                        <div class="progress-header">
                            <i class="bi bi-gear-wide-connected"></i>
                            <span>{{ $t('plugins.housekeeper.progress_status') }}</span>
                            <span class="progress-text">{{ hkStatusText }}</span>
                        </div>
                        <div class="progress-bar-custom">
                            <div class="progress-fill" :style="{ width: hkPctDone + '%' }"></div>
                        </div>
                    </div>
                </div>
            </div>



            <!-- System Info Card -->
            <div class="settings-card">
                <div class="card-header">
                    <i class="bi bi-info-circle-fill"></i>
                    <h3>{{ $t('settings.orthanc_system_info') }}</h3>
                </div>
                <div class="card-body">
                    <div class="info-list">
                        <div class="info-item">
                            <span class="info-label">
                                <i class="bi bi-box-seam"></i>
                                {{ $t('settings.orthanc_version') }}
                            </span>
                            <span class="info-value version">{{ system.Version }}</span>
                        </div>
                        <div class="info-item">
                            <span class="info-label">
                                <i class="bi bi-broadcast"></i>
                                {{ $t('settings.dicom_AET') }}
                            </span>
                            <span class="info-value">{{ system.DicomAet }}</span>
                        </div>
                        <div class="info-item">
                            <span class="info-label">
                                <i class="bi bi-hospital"></i>
                                {{ $t('settings.orthanc_name') }}
                            </span>
                            <span class="info-value">{{ system.Name }}</span>
                        </div>
                        <div class="info-item">
                            <span class="info-label">
                                <i class="bi bi-ethernet"></i>
                                {{ $t('settings.dicom_port') }}
                            </span>
                            <span class="info-value">{{ system.DicomPort }}</span>
                        </div>
                        <div class="info-item">
                            <span class="info-label">
                                <i class="bi bi-arrow-left-right"></i>
                                {{ $t('settings.ingest_transcoding') }}
                            </span>
                            <span class="info-value">
                                <span class="badge"
                                    :class="system.IngestTranscoding !== 'none' ? 'badge-success' : 'badge-secondary'">
                                    {{ system.IngestTranscoding }}
                                </span>
                            </span>
                        </div>
                        <div class="info-item">
                            <span class="info-label">
                                <i class="bi bi-shield-check"></i>
                                {{ $t('settings.overwrite_instances') }}
                            </span>
                            <span class="info-value">
                                <span class="badge"
                                    :class="system.OverwriteInstances ? 'badge-warning' : 'badge-secondary'">
                                    {{ system.OverwriteInstances ? $t('yes') : $t('no') }}
                                </span>
                            </span>
                        </div>
                        <div class="info-item">
                            <span class="info-label">
                                <i class="bi bi-file-zip"></i>
                                {{ $t('settings.storage_compression') }}
                            </span>
                            <span class="info-value">
                                <span class="badge"
                                    :class="system.StorageCompression ? 'badge-success' : 'badge-secondary'">
                                    {{ system.StorageCompression ? $t('yes') : $t('no') }}
                                </span>
                            </span>
                        </div>
                        <div class="info-item">
                            <span class="info-label">
                                <i class="bi bi-eye-slash"></i>
                                {{ $t('settings.read_only') }}
                            </span>
                            <span class="info-value">
                                <span class="badge" :class="system.ReadOnly ? 'badge-warning' : 'badge-secondary'">
                                    {{ system.ReadOnly ? $t('yes') : $t('no') }}
                                </span>
                            </span>
                        </div>
                    </div>
                </div>
            </div>

            <!-- Verbosity Level Card -->
            <div class="settings-card">
                <div class="card-header">
                    <i class="bi bi-terminal-fill"></i>
                    <h3>{{ $t('settings.verbosity_level') }}</h3>
                </div>
                <div class="card-body">
                    <div class="verbosity-selector">
                        <button class="verbosity-btn" :class="{ active: verboseLevel === 'default' }"
                            @click="setVerboseLevel('default')">
                            <i class="bi bi-circle"></i>
                            <span>{{ $t('default') }}</span>
                        </button>
                        <button class="verbosity-btn" :class="{ active: verboseLevel === 'verbose' }"
                            @click="setVerboseLevel('verbose')">
                            <i class="bi bi-circle-half"></i>
                            <span>{{ $t('verbose') }}</span>
                        </button>
                        <button class="verbosity-btn" :class="{ active: verboseLevel === 'trace' }"
                            @click="setVerboseLevel('trace')">
                            <i class="bi bi-circle-fill"></i>
                            <span>{{ $t('trace') }}</span>
                        </button>
                    </div>
                </div>
            </div>
            <!-- Devices Component -->
            <div class="settings-card devices-card">
                <Devices />
            </div>
            <!-- Plugins Card -->
            <div class="settings-card wide">
                <div class="card-header">
                    <i class="bi bi-puzzle-fill"></i>
                    <h3>{{ $t('settings.installed_plugins') }}</h3>
                </div>
                <div class="card-body">
                    <div class="plugins-hint">
                        <i class="bi bi-info-circle"></i>
                        <span>{{ $t('settings.plugins_not_enabled') }}</span>
                        <span class="striked-example">{{ $t('settings.striked_through') }}</span>
                    </div>

                    <div class="plugins-grid">
                        <div v-for="(configuration, plugin) in installedPlugins" :key="plugin" class="plugin-card"
                            :class="{ disabled: !configuration.Enabled }">
                            <div class="plugin-header">
                                <div class="plugin-icon">
                                    <i class="bi bi-puzzle"></i>
                                </div>
                                <div class="plugin-title">
                                    <h4>{{ plugin }}</h4>
                                    <span class="plugin-version">v{{ configuration.Version }}</span>
                                </div>
                                <span class="plugin-status" :class="{ active: configuration.Enabled }">
                                    <i
                                        :class="configuration.Enabled ? 'bi bi-check-circle-fill' : 'bi bi-x-circle-fill'"></i>
                                </span>
                            </div>
                            <p class="plugin-description">{{ configuration.Description }}</p>
                            <a v-if="configuration.RootUri && configuration.Enabled && !tokens.RequiredForLinks"
                                :href="configuration.RootUri" class="plugin-link" target="_blank">
                                <span>{{ $t('open') }}</span>
                                <i class="bi bi-box-arrow-up-right"></i>
                            </a>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>

<style scoped>
.settings-container {
    padding: 24px;
    max-width: 1400px;
    margin: 0 auto;
    background: #f8fafc;
    min-height: 100vh;
}

.settings-header {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    margin-bottom: 24px;
    padding-bottom: 16px;
    border-bottom: 1px solid #e2e8f0;
}

.header-left {
    display: flex;
    flex-direction: column;
    gap: 12px;
}

.header-title {
    display: flex;
    align-items: center;
    gap: 12px;
}

.header-title i {
    font-size: 28px;
    color: #3b82f6;
    background: linear-gradient(135deg, #3b82f6 0%, #8b5cf6 100%);
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
}

.header-title h1 {
    font-size: 24px;
    font-weight: 600;
    color: #1e293b;
    margin: 0;
}

.admin-nav {
    display: flex;
    gap: 10px;
    margin-left: 40px;
}

.nav-btn {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 8px 16px;
    background: white;
    border: 1px solid #e2e8f0;
    border-radius: 8px;
    color: #475569;
    font-size: 13px;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.2s ease;
}

.nav-btn:hover {
    background: #f8fafc;
    border-color: #3b82f6;
    color: #3b82f6;
    transform: translateY(-1px);
    box-shadow: 0 2px 8px rgba(59, 130, 246, 0.15);
}

.nav-btn i {
    font-size: 14px;
}

.header-actions {
    display: flex;
    align-items: center;
    gap: 12px;
}

.settings-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
    gap: 20px;
}

.settings-card {
    background: white;
    border-radius: 12px;
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1), 0 1px 2px rgba(0, 0, 0, 0.06);
    overflow: hidden;
    transition: box-shadow 0.2s ease, transform 0.2s ease;
}

.settings-card:hover {
    box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1), 0 2px 4px rgba(0, 0, 0, 0.06);
}

.settings-card.wide {
    grid-column: 1 / -1;
}

.devices-card {
    grid-column: 1 / -1;
    padding: 20px;
}

.card-header {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 16px 20px;
    background: linear-gradient(135deg, #f1f5f9 0%, #e2e8f0 100%);
    border-bottom: 1px solid #e2e8f0;
}

.card-header i {
    font-size: 18px;
    color: #3b82f6;
}

.card-header h3 {
    font-size: 16px;
    font-weight: 600;
    color: #334155;
    margin: 0;
}

.card-body {
    padding: 20px;
}

.stat-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
    gap: 16px;
}

.stat-item {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 16px;
    background: #f8fafc;
    border-radius: 10px;
    border: 1px solid #e2e8f0;
    transition: all 0.2s ease;
}

.stat-item:hover {
    background: #f1f5f9;
    border-color: #cbd5e1;
    transform: translateY(-2px);
}

.stat-item.wide {
    grid-column: 1 / -1;
    flex-wrap: wrap;
}

.stat-icon {
    width: 40px;
    height: 40px;
    border-radius: 10px;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 18px;
    color: white;
}

.stat-icon.patients {
    background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
}

.stat-icon.studies {
    background: linear-gradient(135deg, #10b981 0%, #059669 100%);
}

.stat-icon.series {
    background: linear-gradient(135deg, #f59e0b 0%, #d97706 100%);
}

.stat-icon.instances {
    background: linear-gradient(135deg, #8b5cf6 0%, #7c3aed 100%);
}

.stat-icon.storage {
    background: linear-gradient(135deg, #64748b 0%, #475569 100%);
}

.stat-info {
    display: flex;
    flex-direction: column;
    gap: 2px;
}

.stat-label {
    font-size: 12px;
    color: #64748b;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.stat-value {
    font-size: 20px;
    font-weight: 700;
    color: #1e293b;
}

.storage-bar {
    width: 100%;
    margin-top: 12px;
}

.storage-bar .progress {
    height: 6px;
    background: #e2e8f0;
    border-radius: 3px;
    overflow: hidden;
}

.storage-bar .progress-bar {
    height: 100%;
    background: linear-gradient(90deg, #3b82f6 0%, #8b5cf6 100%);
    border-radius: 3px;
    transition: width 0.3s ease;
}

.plugin-stats {
    display: flex;
    gap: 16px;
    margin-top: 16px;
    padding-top: 16px;
    border-top: 1px solid #e2e8f0;
}

.plugin-stat-item {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 10px 14px;
    background: #fef3c7;
    border-radius: 8px;
    font-size: 13px;
    color: #92400e;
}

.plugin-stat-item i {
    font-size: 16px;
}

.plugin-stat-item strong {
    color: #78350f;
    font-weight: 600;
}

.housekeeper-progress {
    margin-top: 16px;
    padding: 16px;
    background: #f0fdf4;
    border-radius: 10px;
    border: 1px solid #bbf7d0;
}

.progress-header {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 10px;
    font-size: 13px;
    color: #166534;
}

.progress-header i {
    color: #22c55e;
}

.progress-text {
    margin-left: auto;
    font-weight: 600;
    color: #15803d;
}

.progress-bar-custom {
    height: 8px;
    background: #dcfce7;
    border-radius: 4px;
    overflow: hidden;
}

.progress-fill {
    height: 100%;
    background: linear-gradient(90deg, #22c55e 0%, #16a34a 100%);
    border-radius: 4px;
    transition: width 0.3s ease;
}

.info-list {
    display: flex;
    flex-direction: column;
    gap: 12px;
}

.info-item {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 12px 16px;
    background: #f8fafc;
    border-radius: 8px;
    border: 1px solid #e2e8f0;
    transition: all 0.2s ease;
}

.info-item:hover {
    background: #f1f5f9;
}

.info-label {
    display: flex;
    align-items: center;
    gap: 10px;
    font-size: 14px;
    color: #475569;
}

.info-label i {
    color: #64748b;
    font-size: 16px;
}

.info-value {
    font-size: 14px;
    font-weight: 500;
    color: #1e293b;
}

.info-value.version {
    font-family: 'SF Mono', Monaco, monospace;
    background: #e0e7ff;
    color: #3730a3;
    padding: 4px 10px;
    border-radius: 6px;
    font-size: 13px;
}

.badge {
    display: inline-flex;
    align-items: center;
    padding: 4px 10px;
    border-radius: 20px;
    font-size: 12px;
    font-weight: 500;
}

.badge-success {
    background: #dcfce7;
    color: #166534;
}

.badge-secondary {
    background: #f1f5f9;
    color: #64748b;
}

.badge-warning {
    background: #fef3c7;
    color: #92400e;
}

.verbosity-selector {
    display: flex;
    gap: 12px;
}

.verbosity-btn {
    flex: 1;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 8px;
    padding: 20px;
    background: #f8fafc;
    border: 2px solid #e2e8f0;
    border-radius: 10px;
    cursor: pointer;
    transition: all 0.2s ease;
}

.verbosity-btn:hover {
    background: #f1f5f9;
    border-color: #cbd5e1;
}

.verbosity-btn.active {
    background: linear-gradient(135deg, #eff6ff 0%, #dbeafe 100%);
    border-color: #3b82f6;
    color: #1d4ed8;
}

.verbosity-btn i {
    font-size: 24px;
}

.verbosity-btn span {
    font-size: 13px;
    font-weight: 500;
}

.plugins-hint {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 16px;
    padding: 12px 16px;
    background: #f8fafc;
    border-radius: 8px;
    font-size: 13px;
    color: #64748b;
}

.plugins-hint i {
    color: #3b82f6;
}

.striked-example {
    text-decoration: line-through;
    color: #94a3b8;
    margin-left: 4px;
}

.plugins-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
    gap: 16px;
}

.plugin-card {
    background: #f8fafc;
    border: 1px solid #e2e8f0;
    border-radius: 10px;
    padding: 16px;
    transition: all 0.2s ease;
}

.plugin-card:hover {
    background: white;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.08);
    transform: translateY(-2px);
}

.plugin-card.disabled {
    opacity: 0.6;
    background: #f1f5f9;
}

.plugin-card.disabled .plugin-icon {
    background: #cbd5e1;
}

.plugin-header {
    display: flex;
    align-items: center;
    gap: 12px;
    margin-bottom: 10px;
}

.plugin-icon {
    width: 36px;
    height: 36px;
    background: linear-gradient(135deg, #3b82f6 0%, #8b5cf6 100%);
    border-radius: 8px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: white;
    font-size: 16px;
}

.plugin-title {
    flex: 1;
}

.plugin-title h4 {
    font-size: 14px;
    font-weight: 600;
    color: #1e293b;
    margin: 0;
}

.plugin-version {
    font-size: 12px;
    color: #64748b;
    font-family: 'SF Mono', Monaco, monospace;
}

.plugin-status {
    font-size: 18px;
}

.plugin-status.active {
    color: #22c55e;
}

.plugin-status:not(.active) {
    color: #94a3b8;
}

.plugin-description {
    font-size: 13px;
    color: #64748b;
    line-height: 1.5;
    margin: 0 0 12px 0;
}

.plugin-link {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 6px 12px;
    background: #3b82f6;
    color: white;
    text-decoration: none;
    border-radius: 6px;
    font-size: 13px;
    font-weight: 500;
    transition: all 0.2s ease;
}

.plugin-link:hover {
    background: #2563eb;
    transform: translateX(2px);
}

.plugin-link i {
    font-size: 12px;
}

:deep(.menu-item.language-switcher) {
    background: white;
    border: 1px solid #e2e8f0;
    border-radius: 8px;
    padding: 8px 16px;
    color: #475569;
}

:deep(.menu-item.language-switcher:hover) {
    background: #f8fafc;
    border-color: #3b82f6;
    color: #3b82f6;
}

@media (max-width: 768px) {
    .settings-container {
        padding: 16px;
    }

    .settings-grid {
        grid-template-columns: 1fr;
    }

    .stat-grid {
        grid-template-columns: repeat(2, 1fr);
    }

    .verbosity-selector {
        flex-direction: column;
    }

    .plugins-grid {
        grid-template-columns: 1fr;
    }

    .admin-nav {
        margin-left: 0;
        flex-wrap: wrap;
    }

    .settings-header {
        flex-direction: column;
        gap: 16px;
    }
}
</style>