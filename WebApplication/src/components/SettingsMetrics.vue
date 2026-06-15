<script>

import { mapState } from "vuex"
import api from "../orthancApi"


export default {
    props: [],
    emits: [],
    data() {
        return {
            metrics: {},
            groups: {},
            cancelRefreshMetrics: null,
        };
    },
    async mounted() {
        console.log("metrics list mounted");
        this.refresh(true);
    },
    async unmounted() {
        console.log("metrics list unmounted");
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
        async refresh(autoRefresh) {
            this.metrics = await api.getMetrics();
            this.groups = {
                "Statistics": ["orthanc_count_patients", "orthanc_count_studies", "orthanc_count_series", "orthanc_count_instances", "orthanc_disk_size_mb", "orthanc_uncompressed_size_mb"],
                "Jobs": ["orthanc_jobs_running", "orthanc_jobs_pending", "orthanc_jobs_completed", "orthanc_jobs_success", "orthanc_jobs_failed"],
                "Rest API": ["orthanc_available_http_threads_count", "orthanc_rest_api_active_requests", "orthanc_rest_api_duration_ms"],
                "DICOM Protocol": ["orthanc_available_dicom_threads"],
                "System": ["orthanc_up_time_s", "orthanc_last_change", "orthanc_logged_errors_count", "orthanc_logged_warnings_count", "orthanc_memory_trimming_duration_ms"],
            };

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
                    // this.groups["Others"].push(k);
                }
            }

            if (autoRefresh) {
                this.cancelRefreshMetrics = setTimeout(() => {this.refresh(true);}, 5000);
            }
        },
        getValue(key) {
            if (key == 'orthanc_up_time_s') {
                return this.elapsed_(this.metrics['orthanc_up_time_s']['value'], '')
            } else {
                return this.metrics[key]['value'];
            }
        },
        elapsed(key) {
            const elapsed = Math.round((this.metrics['orthanc_up_time_s']['timestamp'] - this.metrics[key]['timestamp']) / 1000);
            return this.elapsed_(elapsed, ' ago');
        },
        elapsed_(duration, suffix=' ago') {
            
            if (duration <= 1) {
                return 'now';
            } 
            
            const intervals = [
                { label: "d", max: 86400 },    
                { label: "h", max: 3600 },
                { label: "m", max: 60 },
                { label: "s", max: -1 },
            ];

            let remaining = duration;
            const parts = [];

            for (const interval of intervals) {
                if (remaining > interval.max) {
                    const value = Math.abs(Math.floor(remaining / interval.max));
                    remaining %= interval.max;
                    parts.push(`${value}${interval.label}`);
                }
                
                if (parts.length >= 2) 
                    break; // Limit to 2 units (e.g., "1h42m")
            }

            if (parts.length === 0) 
                return "now";
            return `${parts.join("")}${suffix}`;
        }
    },
    watch: {
        async isConfigurationLoaded(newValue, oldValue) {
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            isConfigurationLoaded: state => state.configuration.loaded,
        }),
    },
}
</script>
<template>
    <div class="settings">
        <div>
            <h5>{{ $t('settings.metrics') }}</h5>
            <button class="btn btn-primary" @click="refresh()">{{ $t('settings.metrics_refresh') }}</button>
            <table class="table table-sm">
                <tbody v-for="(groupList, groupName) in groups" :key="groupName">
<!--                    <tr v-for="(v, k) in metrics" :key="k">
                        <th scope="row" class="w-50 header">{{ k }}</th>
                        <td class="value">{{ getValue(k) }}</td>
                        <td class="w-15 timestamp">{{ elapsed(k) }}</td>
                    </tr> -->
                    <tr class="mt-2"><td colspan="3">{{ groupName }}
                    </td>
                    </tr>
                    <template v-for="m in groupList" :key="m">
                        <tr v-if="m in metrics">
                            <th scope="row" class="w-50 header">{{ m }}</th>
                            <td class="value">{{ getValue(m) }}</td>
                            <td class="w-15 timestamp">{{ elapsed(m) }}</td>
                        </tr>
                    </template>
                </tbody>
            </table>
            <button class="btn btn-primary" @click="refresh()">{{ $t('settings.metrics_refresh') }}</button>
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