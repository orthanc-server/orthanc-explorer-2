<script>

import { mapState } from "vuex"
import api from "../orthancApi"


export default {
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
        };
    },
    async mounted() {
        this.verboseLevel = await api.getVerboseLevel();
        await this.loadPluginsStatus();
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
                return this.hkLastProcessedChange +" / " + this.hkLastChangeToProcess;
            } else {
                return this.$t('plugins.housekeeper.completed')
            }
        }
    },
}
</script>
<template>
    <div class="settings">
        <div>
            <h5>{{ $t('settings.statistics') }}</h5>
            <table class="table">
                <tbody>
                    <tr>
                        <th scope="row" class="w-50 header"># {{ $t('patients') }}</th>
                        <td v-if="!hasMaxPatientCount" class="value">{{ statistics.CountPatients }}</td>
                        <td v-if="hasMaxPatientCount" class="value">{{ statistics.CountPatients }} / {{
                system.MaximumPatientCount }} ({{ system.MaximumStorageMode }})</td>
                    </tr>
                    <tr>
                        <th scope="row" class="w-50 header"># {{ $t('studies') }}</th>
                        <td class="value">{{ statistics.CountStudies }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header"># {{ $t('series_plural') }}</th>
                        <td class="value">{{ statistics.CountSeries }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header"># {{ $t('instances') }}</th>
                        <td class="value">{{ statistics.CountInstances }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{ $t('settings.storage_size') }}</th>
                        <td v-if="!hasMaxStorageSize" class="value">{{ totalDiskSize }}</td>
                        <td v-if="hasMaxStorageSize" class="value">{{ totalDiskSize }} / {{ maxStorageSize }} ({{
                system.MaximumStorageMode }})</td>
                    </tr>
                    <tr v-if="hasDelayedDeletionPlugin">
                        <th scope="row" class="w-50 header"># {{ $t('plugins.delayed_deletion.pending_files_count') }}
                        </th>
                        <td class="value">{{ delayedDeletionPendingFilesCount }}</td>
                    </tr>
                    <tr v-if="hasAdvancedStorageDelayedDeletion">
                        <th scope="row" class="w-50 header"># {{ $t('plugins.advanced_storage.pending_deletion_files_count') }}
                        </th>
                        <td class="value">{{ advancedStoragePendingDeletionFilesCount }}</td>
                    </tr>
                    <tr v-if="hasHousekeeperPlugin">
                        <th scope="row" class="w-50 header">{{ $t('plugins.housekeeper.progress_status') }}
                        </th>
                        <td class="value">
                            <div class="progress mt-1 mb-1" style="width:100%">
                                <div class="progress-bar bg-success" role="progressbar"
                                    v-bind:style="'width: ' + this.hkPctDone + '%'"><span v-if="hkPctDone > 50">{{ hkStatusText }}</span></div>
                                <div class="progress-bar bg-secondary" role="progressbar"
                                    v-bind:style="'width: ' + this.hkPctRemaining + '%'"><span v-if="hkPctDone <= 50">{{ hkStatusText }}</span></div>
                            </div>
                        </td>
                    </tr>
                </tbody>
            </table>
        </div>
        <div>
            <h5>{{ $t('settings.orthanc_system_info') }}</h5>
            <table class="table">
                <tbody>
                    <tr>
                        <th scope="row" class="w-50 header">{{ $t('settings.orthanc_version') }}</th>
                        <td class="value">{{ system.Version }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{ $t('settings.dicom_AET') }}</th>
                        <td class="value">{{ system.DicomAet }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{ $t('settings.orthanc_name') }}</th>
                        <td class="value">{{ system.Name }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{ $t('settings.dicom_port') }}</th>
                        <td class="value">{{ system.DicomPort }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{ $t('settings.ingest_transcoding') }}</th>
                        <td class="value">{{ system.IngestTranscoding }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{ $t('settings.overwrite_instances') }}</th>
                        <td class="value">{{ system.OverwriteInstances }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{ $t('settings.storage_compression') }}</th>
                        <td class="value">{{ system.StorageCompression }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{ $t('settings.read_only') }}</th>
                        <td class="value">{{ system.ReadOnly }}</td>
                    </tr>
                </tbody>
            </table>
        </div>
        <div>
            <h5>{{ $t('settings.verbosity_level') }}</h5>
            <div>
                <button type="button" class="btn mx-2" @click="setVerboseLevel('default')"
                    :class="{ 'btn-primary': verboseLevel == 'default', 'btn-secondary': verboseLevel != 'default' }">{{
                $t('default') }}</button>
                <button type="button" class="btn mx-2" @click="setVerboseLevel('verbose')"
                    :class="{ 'btn-primary': verboseLevel == 'verbose', 'btn-secondary': verboseLevel != 'verbose' }">{{
                $t('verbose') }}</button>
                <button type="button" class="btn mx-2" @click="setVerboseLevel('trace')"
                    :class="{ 'btn-primary': verboseLevel == 'trace', 'btn-secondary': verboseLevel != 'trace' }">{{
                $t('trace') }}</button>
            </div>
        </div>
        <div>
            <h5>{{ $t('settings.installed_plugins') }}</h5>
            <p class="m-2">{{ $t('settings.plugins_not_enabled') }} <span style="text-decoration: line-through;">{{
                $t('settings.striked_through') }}</span></p>
            <table class="table">
                <tbody>
                    <tr v-for="(configuration, plugin) in installedPlugins" :key="plugin"
                        :class="{ 'disabled-plugin': !configuration.Enabled }">
                        <th scope="row" class="w-25 header">{{ plugin }}</th>
                        <td class="w-50 value">{{ configuration.Description }}</td>
                        <td class="w-15 value">{{ configuration.Version }}</td>
                        <td class="w-15 value" v-if="!this.tokens.RequiredForLinks">
                            <!-- If tokens are required for links, no need to display links to plugin UI, they usually don't support links and won't work -->
                            <a v-if="configuration.RootUri && configuration.Enabled" type="button"
                                class="btn btn-primary" v-bind:href="configuration.RootUri">{{ $t('open') }}</a>
                        </td>
                    </tr>
                </tbody>
            </table>
        </div>
    </div>
</template>
<style scoped>
h5 {
    margin-top: 50px;
    margin-bottom: 15px;
    margin-left: 50px;
}

h6 {
    font-weight: 600;
}

.settings {
    text-align: left;
}

.w-15 {
    width: 15%;
}

.disabled-plugin {
    text-decoration: line-through;
}

.header {
    padding-left: 15px;
}

.value {
    text-align: right;
    padding-right: 15px;
}

.instructions {
    margin-left: 1rem;
}

.instructions-text {
    font-size: smaller;
}


</style>