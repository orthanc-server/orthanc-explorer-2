<script>

import { mapState } from "vuex"
import api from "../orthancApi"
import MetricsGraph from "./MetricsGraph.vue";
import dateHelpers from "../helpers/date-helpers.js";

// global data (not reactive) shared between this component and the MetreicsGraph
document.metricsHistory = {};
document.metricsLowerTimestamp = null;
document.metricsHigherTimestamp = null;

export default {
    props: [],
    emits: [],
    data() {
        return {
            metrics: {},
            groups: {},
            cancelRefreshMetrics: null,
            showGraphs: true,
            autoRefresh: true,
            historyGraphDurationInSeconds: 180,
            autoRefreshIntervalInSeconds: 5
        };
    },
    async mounted() {
        // console.log("metrics list mounted");
        this.refresh(false);
    },
    async unmounted() {
        // console.log("metrics list unmounted");
        clearTimeout(this.cancelRefreshMetrics);
        this.cancelRefreshMetrics = null;
    },
    methods: {
        _pushInGroup(metricName, groupName) {
            if (!(groupName in this.groups)) {
                this.groups[groupName] = []
            }
            this.groups[groupName].push(metricName);
        },
        async refresh(isManualRefresh) {
            this.metrics = await api.getMetrics();
            document.metricsHigherTimestamp = this.metrics['orthanc_up_time_s']['timestamp'];
            document.metricsLowerTimestamp = document.metricsHigherTimestamp - this.historyGraphDurationInSeconds * 1000;

            this.groups = {
                "Statistics": ["orthanc_count_patients", "orthanc_count_studies", "orthanc_count_series", "orthanc_count_instances", "orthanc_disk_size_mb", "orthanc_uncompressed_size_mb"],
                "Jobs": ["orthanc_jobs_running", "orthanc_jobs_pending", "orthanc_jobs_completed", "orthanc_jobs_success", "orthanc_jobs_failed"],
                "Rest API": ["orthanc_available_http_threads_count", "orthanc_rest_api_active_requests", "orthanc_rest_api_duration_ms"],
                "DICOM Protocol": ["orthanc_available_dicom_threads"],
                "System": ["orthanc_up_time_s", "orthanc_last_change", "orthanc_logged_errors_count", "orthanc_logged_warnings_count", "orthanc_memory_trimming_duration_ms"],
            };

            for (const [k, v] of Object.entries(this.metrics)) {
                if (!(k in document.metricsHistory)) {
                    document.metricsHistory[k] = [];
                }
                const timestamp = this.metrics[k]['timestamp'];
                if (k == 'orthanc_available_dicom_threads') {
                    console.log("stop");
                }
                if (document.metricsHistory[k].length == 0                              // if this is the first measure for the graph
                  || (timestamp > document.metricsHistory[k].at(-1)['x']))              // or if the value has been updated
                {
                    document.metricsHistory[k].push({ x: this.metrics[k]['timestamp'], y: this.metrics[k]['value'] })
                }
                else if (document.metricsHistory[k].length > 0 
                  && ((document.metricsHigherTimestamp - document.metricsHistory[k].at(-1)['x']) > (this.autoRefreshIntervalInSeconds * 1000))) // if the value has not been updated for 5 seconds)
                {
                    // add a "fake" point for graph
                    document.metricsHistory[k].push({ x: document.metricsHigherTimestamp, y: this.metrics[k]['value'] })
                }

                if (this.showGraphs) {
                    this.messageBus.emit("refresh-metrics-" + k); // even if there are no new values, the limit timestamps have been updated so we need to refresh the graph
                }
            }

            // automatic add in groups (based on prefix)
            for (const [k, v] of Object.entries(this.metrics)) {
                if (k.startsWith("orthanc_dicomweb_")) {
                    this._pushInGroup(k, "DICOMweb plugin");
                }
                else if (k.startsWith("orthanc_transfers_")) {
                    this._pushInGroup(k, "Transfers plugin");
                }
            }

            let metricsInGroup = [];
            for (const [k, v] of Object.entries(this.groups)) {
                for (const m of v) {
                    metricsInGroup.push(m);
                }
            }

            for (const [k, v] of Object.entries(this.metrics)) {
                if (metricsInGroup.indexOf(k) == -1) {
                    this._pushInGroup(k, "Others");
                }
            }

            if (!isManualRefresh && this.autoRefresh) {
                this.cancelRefreshMetrics = setTimeout(() => { this.refresh(false); }, this.autoRefreshIntervalInSeconds * 1000);
            }
        },
        formatFloatString(str) {
            const num = parseFloat(str);
            if (!isNaN(num) && str.includes('.')) {
                return num.toFixed(1);
            }
            return str;
        },
        getValue(key) {
            if (key == 'orthanc_up_time_s') {
                return dateHelpers.getElapsedDurationFromMilliseconds(this.metrics['orthanc_up_time_s']['value'], '')
            } else {
                return this.formatFloatString(this.metrics[key]['value']) + this.getReference(key);  // non float strings are kept untouched !
            }
        },
        elapsed(key) {
            const elapsed = Math.round((this.metrics['orthanc_up_time_s']['timestamp'] - this.metrics[key]['timestamp']) / 1000);
            return dateHelpers.getElapsedDurationFromMilliseconds(elapsed);
        },
        getReference(key) {
            const references = {
                "orthanc_jobs_running": "ConcurrentJobs",
                "orthanc_available_http_threads_count": "HttpThreadsCount",
                "orthanc_available_dicom_threads": "DicomThreadsCount",
                "orthanc_dicom_parser_available_threads": "DicomParserThreads",
                "orthanc_dicom_parser_cache_size_mb": "DicomParserCacheSize",
                "orthanc_dicom_parser_memory_usage_mb": "orthanc_dicom_parser_memory_capacity_mb",
                "orthanc_dicom_parser_memory_max_usage_mb": "orthanc_dicom_parser_memory_capacity_mb",
                "orthanc_seq_reader_available_threads": "SequentialDicomReaderThreads",
                "orthanc_storage_available_threads": "StorageLoaderThreads",
                "orthanc_storage_cache_size_mb": "MaximumStorageCacheSize",
                "orthanc_storage_memory_usage_mb": "orthanc_storage_memory_capacity_mb",
                "orthanc_storage_memory_max_usage_mb": "orthanc_storage_memory_capacity_mb",
                "orthanc_transcoder_available_threads": "TranscoderThreads",
                "orthanc_transcoder_cache_size_mb": "TranscoderCacheSize",
                "orthanc_transcoder_memory_usage_mb": "orthanc_transcoder_memory_capacity_mb",
                "orthanc_transcoder_memory_max_usage_mb": "orthanc_transcoder_memory_capacity_mb",
            }

            if (key in references) {
                if (references[key] in this.system) {
                    return " / " + this.system[references[key]];
                } else if (references[key] in this.metrics) {
                    return " / " + this.metrics[references[key]]['value'];
                }
            }
            return "";
        }
    },
    watch: {
        autoRefresh(newValue, oldValue) {
            if (!newValue && this.cancelRefreshMetrics) {
                clearTimeout(this.cancelRefreshMetrics);
                this.cancelRefreshMetrics = null;
            }
            if (newValue && !this.cancelRefreshMetrics) {
                this.refresh(false);
            }
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            system: state => state.configuration.system
        }),
    },
    components: { MetricsGraph }
}
</script>
<template>
    <div class="settings">
        <div>
            <h5>{{ $t('settings.metrics') }}</h5>
            <div class="container">
                <div class="row">
                    <div class="col-4"><button class="btn btn-primary" @click="refresh(true)">{{
                        $t('settings.metrics_refresh') }}</button></div>
                    <div class="col-4">
                        <div class="form-check form-check-reverse form-switch">
                            <input class="form-check-input" type="checkbox" role="switch" id="switchAutoRefresh" v-model="autoRefresh">
                            <label class="form-check-label" for="switchAutoRefresh">auto-refresh</label>
                        </div>

                    </div>
                    <div class="col-4">
                        <div class="form-check form-check-reverse form-switch">
                            <input class="form-check-input" type="checkbox" role="switch" id="switchShowGraph" v-model="showGraphs">
                            <label class="form-check-label" for="switchShowGraph">show graphs</label>
                        </div>
                    </div>
                </div>

            </div>
            <table class="table table-sm">
                <tbody v-for="(groupList, groupName) in groups" :key="groupName">
                    <tr class="mt-2">
                        <td colspan="4">{{ groupName }}
                        </td>
                    </tr>
                    <template v-for="m in groupList" :key="m">
                        <tr v-if="m in metrics">
                            <th scope="row" class="w-50 header">{{ m }}</th>
                            <td class="value">{{ getValue(m) }}</td>
                            <td class="w-10 timestamp">{{ elapsed(m) }}</td>
                            <td v-show="showGraphs" class="w-15 chart">
                                <MetricsGraph :metricsName="m"></MetricsGraph>
                            </td>
                        </tr>
                    </template>
                </tbody>
            </table>
            <button class="btn btn-primary" @click="refresh(true)">{{ $t('settings.metrics_refresh') }}</button>
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

.w-10 {
    width: 15%;
}

.w-15 {
    width: 15%;
}

.chart {
    background-color: dimgrey;
}

.header {
    padding-left: 20px;
    font-weight: 400;
    font-size: small;
}

.value {
    text-align: right;
    padding-right: 10px;
    font-weight: 400;
    font-size: small;
}

.timestamp {
    text-align: left;
    padding-right: 10px;
    font-weight: 400;
    font-size: smaller;
    opacity: 0.5;
}
</style>