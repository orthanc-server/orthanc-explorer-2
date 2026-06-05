<script>

import { mapState } from "vuex"
import api from "../orthancApi"


export default {
    props: [],
    emits: [],
    data() {
        return {
            metrics: {},
        };
    },
    async mounted() {
        await this.refresh();
    },
    methods: {
        async refresh() {
            this.metrics = await api.getMetrics();
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
                <tbody>
                    <tr v-for="(v, k) in metrics" :key="k">
                        <th scope="row" class="w-50 header">{{ k }}</th>
                        <td class="value">{{ getValue(k) }}</td>
                        <td class="w-15 timestamp">{{ elapsed(k) }}</td>
                    </tr>
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
    padding-left: 10px;
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