<script>

import { mapState } from "vuex"
import api from "../orthancApi"


export default {
    props: [],
    emits: [],
    data() {
        return {
            verboseLevel: "default"
        };
    },
    async mounted() {
        this.verboseLevel = await api.getVerboseLevel();
    },
    methods: {
        async setVerboseLevel(level) {
            this.verboseLevel = level;
            await api.setVerboseLevel(level);
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            tokens: state => state.configuration.tokens,
            statistics: state => state.studies.statistics,
            system: state => state.configuration.system,
            installedPlugins: state => state.configuration.installedPlugins,
        }),
        totalDiskSize() {
            if (this.statistics.TotalDiskSizeMB > 1024*1024) {
                return (Math.round(this.statistics.TotalDiskSizeMB/(1024*1024)*100) / 100) + " TB"; 
            }
            else if (this.statistics.TotalDiskSizeMB > 1024) {
                return (Math.round(this.statistics.TotalDiskSizeMB/1024*100) / 100) + " GB"; 
            }
            return this.statistics.TotalDiskSizeMB + " MB";
        }
    },
}
</script>
<template>
    <div class="settings">
        <div>
            <h5>{{$t('statistics')}}</h5>
            <table class="table">
                <tbody>
                    <tr>
                        <th scope="row" class="w-50 header"># {{$t('studies')}}</th>
                        <td class="value">{{ statistics.CountStudies }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header"># {{$t('series_plural')}}</th>
                        <td class="value">{{ statistics.CountSeries }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header"># {{$t('instances')}}</th>
                        <td class="value">{{ statistics.CountInstances }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{$t('storage_size')}}</th>
                        <td class="value">{{ totalDiskSize }}</td>
                    </tr>
                </tbody>
            </table>
        </div>
        <div>
            <h5>{{$t('orthanc_system_info')}}</h5>
            <table class="table">
                <tbody>
                    <tr>
                        <th scope="row" class="w-50 header">{{$t('orthanc_version')}}</th>
                        <td class="value">{{ system.Version }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{$t('dicom_AET')}}</th>
                        <td class="value">{{ system.DicomAet }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{$t('orthanc_name')}}</th>
                        <td class="value">{{ system.Name }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{$t('dicom_port')}}</th>
                        <td class="value">{{ system.DicomPort }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{$t('ingest_transcoding')}}</th>
                        <td class="value">{{ system.IngestTranscoding }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{$t('overwrite_instances')}}</th>
                        <td class="value">{{ system.OverwriteInstances }}</td>
                    </tr>
                    <tr>
                        <th scope="row" class="header">{{$t('storage_compression')}}</th>
                        <td class="value">{{ system.StorageCompression }}</td>
                    </tr>
                </tbody>
            </table>
        </div>
        <div>
            <h5>{{$t('verbosity_level')}}</h5>
            <div>
            <button type="button" class="btn mx-2" @click="setVerboseLevel('default')" :class="{'btn-primary': verboseLevel=='default', 'btn-secondary': verboseLevel != 'default'}">{{$t('default')}}</button>
            <button type="button" class="btn mx-2" @click="setVerboseLevel('verbose')" :class="{'btn-primary': verboseLevel=='verbose', 'btn-secondary': verboseLevel != 'verbose'}">{{$t('verbose')}}</button>
            <button type="button" class="btn mx-2" @click="setVerboseLevel('trace')" :class="{'btn-primary': verboseLevel=='trace', 'btn-secondary': verboseLevel != 'trace'}">{{$t('trace')}}</button>
            </div>
        </div>
        <div>
            <h5>{{$t('installed_plugins')}}</h5>
            <p class="m-2">{{$t('plugins_not_enabled')}} <span style="text-decoration: line-through;">{{$t('striked_through')}}</span></p>
            <table class="table">
                <tbody>
                    <tr v-for="(configuration, plugin) in installedPlugins" :key="plugin"
                    :class="{'disabled-plugin': !configuration.Enabled}">
                        <th scope="row" class="w-25 header">{{ plugin }}</th>
                        <td class="w-50 value">{{ configuration.Description }}</td>
                        <td class="w-15 value">{{ configuration.Version }}</td>
                        <td class="w-10 value" v-if="!this.tokens.RequiredForLinks"><!-- If tokens are required for links, no need to display links to plugin UI, they usually don't support links and won't work -->
                            <a v-if="configuration.RootUri && configuration.Enabled" type="button" class="btn btn-primary" v-bind:href="configuration.RootUri">{{$t('open')}}</a>
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

.settings {
    text-align: left;
}
.w-15 {
    width: 15%;
}
.w-10 {
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
</style>