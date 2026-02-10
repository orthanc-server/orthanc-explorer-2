<script>
import api from "../orthancApi"
import { mapState } from "vuex"
import dateHelpers from "../helpers/date-helpers"

export default {
    props: ['resourceId'],
    data() {
        return {
            logs: {},
            auditLogsTimeRange: null,
            updatingRouteWithoutReload: false,
            currentFilters: {}
        };
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            isConfigurationLoaded: state => state.configuration.loaded,
        }),
        showResource() {
            return true;
            // return this.resourceId === undefined;
        },
        showUser() {
            return true;
            // return this.resourceId !== undefined;
        },
        isDarkMode() {
            // hack to switch the theme: get the value from our custom css
            let bootstrapTheme = document.documentElement.getAttribute("data-bs-theme"); // for production
            bootstrapTheme = getComputedStyle(document.documentElement).getPropertyValue('--bootstrap-theme');  // for dev
            console.log("DatePicker color mode is ", bootstrapTheme);
            return bootstrapTheme == "dark";
        },
    },
    watch: {
        '$route': async function () { // the watch is used when, e.g, clicking on the back button
            if (!this.updatingRouteWithoutReload) {
                this.updateFromRoute(this.$route.query);
            }
        },
        isConfigurationLoaded(newValue, oldValue) {
            // this is called when opening the page (with a filter or not)
            this.updateFromRoute(this.$route.query);
        },
        async auditLogsTimeRange(newValue, oldValue) {
            //  make a copy otherwise, the router does not see that the object has changed 
            let query = { ...this.$route.query };

            if (this.auditLogsTimeRange && this.auditLogsTimeRange.length >= 2) {
                query["from-timestamp"] = this.auditLogsTimeRange[0].toISOString();
                query["to-timestamp"] = this.auditLogsTimeRange[1].toISOString();
            } else {
                delete query["from-timestamp"];
                delete query["to-timestamp"];
            }

            this.$router.push({ name: 'audit-logs', query: query });
        }
    },
    async mounted() {
        this.updateFromRoute(this.$route.query);
    },
    methods: {
        async updateFromRoute(filters) {
            this.logs = [];
            // console.log(filters);

            const _logs = await api.getAuditLogs(filters);
            this.currentFilters = filters;
            let lastUploadInstanceResourceId = null;
            let countInstancesInGroup = 0;
            if (_logs) {
                for (const log of _logs) {
                    if (log['Action'] == "uploaded-instance") {
                        if (log['ResourceId'] != lastUploadInstanceResourceId) {
                            this.logs.push(log);
                            countInstancesInGroup = 1;
                        } else {
                            countInstancesInGroup++;
                            this.logs[this.logs.length - 1]["Action"] = "uploaded-instances";
                            this.logs[this.logs.length - 1]["JsonLogData"] = {
                                "Count": countInstancesInGroup,
                                "Last": log['Timestamp']
                            }
                        }
                        lastUploadInstanceResourceId = log['ResourceId'];
                    } else {
                        this.logs.push(log);
                        uploadedInstanceGroup = [];
                    }
                }
            }
        },
        async downloadAsCsv() {
            api.getAuditLogs(this.currentFilters, true);
        }
    }
}
</script>

<template>
    <div>
        <!-- <a v-if="!expanded" @click="toggleLogs($event)" href="#">{{ $t('audit_logs.expand_logs') }}</a>
        <a v-if="expanded" @click="toggleLogs($event)" href="#">{{ $t('audit_logs.hide_logs') }}</a> -->
        <div class="row">
            <div class="col-3">
                Select date range:
            </div>
            <div class="col-6">
                <Datepicker v-model="auditLogsTimeRange" :range="true" :enable-time-picker="true"
                    format="yyyy-MM-dd HH:mm:ss" preview-format="yyyy-MM-dd HH:mm:ss" arrow-navigation
                    :highlight="{ weekdays: [6, 0] }" :dark="isDarkMode">
                </Datepicker>

            </div>
            <div class="col-3">
                <button type="button" class="btn btn-primary btn-sm m-1" @click="downloadAsCsv()">{{
                    $t('audit_logs.download_as_csv') }}</button>
            </div>
        </div>
        <div class="text-end">

        </div>
        <table class="table table-responsive table-sm audit-logs-table">
            <thead>
                <tr>
                    <th width="15%" scope="col" class="audit-logs-table-header cut-text text-center"
                        data-bs-toggle="tooltip" :title="$t('audit_logs.column_title_time_stamp')">{{
                            $t('audit_logs.column_title_time_stamp') }}</th>
                    <th v-if="showUser" width="15%" scope="col" class="audit-logs-table-header cut-text text-center"
                        data-bs-toggle="tooltip" :title="$t('audit_logs.column_title_user_id')">{{
                            $t('audit_logs.column_title_user_id') }}</th>
                    <th v-if="showResource" width="15%" scope="col" class="audit-logs-table-header cut-text text-center"
                        data-bs-toggle="tooltip" :title="$t('audit_logs.column_title_resource_id')">
                        {{ $t('audit_logs.column_title_resource_id') }}</th>
                    <th width="15%" scope="col" class="audit-logs-table-header cut-text text-center"
                        data-bs-toggle="tooltip" :title="$t('audit_logs.column_title_action')">
                        {{ $t('audit_logs.column_title_action') }}</th>
                    <th width="15%" scope="col" class="audit-logs-table-header cut-text text-center"
                        data-bs-toggle="tooltip" :title="$t('audit_logs.column_title_log_data')">
                        {{ $t('audit_logs.column_title_log_data') }}</th>
                </tr>
            </thead>
            <tr v-for="log in logs" :key="log">
                <td class="cut-text">{{ log.Timestamp }}</td>
                <td v-if="showUser" class="cut-text">
                    <router-link class="router-link" :to="'/audit-logs?user-id=' + log.UserId">
                        <span v-if="log.UserName">{{ log.UserName }}</span>
                        <span v-else>{{ log.UserId }}</span>
                    </router-link>
                </td>
                <td v-if="showResource" class="cut-text">
                    <router-link class="router-link" :to="'/audit-logs?resource-id=' + log.ResourceId">
                        {{ log.ResourceId }}
                    </router-link>
                </td>
                <td class="cut-text">{{ log.Action }}</td>
                <td v-if="log.Action == 'new-study-from-anonymization-job'" class="log-data">
                    <span class="log-data-title">source</span>: <router-link class="router-link"
                        :to="'/audit-logs?resource-id=' + log.JsonLogData['SourceResourceId']">
                        {{ log.JsonLogData['SourceResourceId'] }}
                    </router-link>{{ }}
                </td>
                <td v-else-if="log.Action == 'success-anonymization-job' || log.Action == 'success-modification-job'"
                    class="log-data">
                    <span class="log-data-title">output</span>: <router-link class="router-link"
                        :to="'/audit-logs?resource-id=' + log.JsonLogData['ModifiedResourceId']">
                        {{ log.JsonLogData['ModifiedResourceId'] }}
                    </router-link>{{ }}
                </td>
                <td v-else-if="log.Action == 'uploaded-instance'" class="log-data">
                    instance-id: {{ log.JsonLogData }}
                </td>
                <td v-else-if="log.Action == 'uploaded-instances'" class="log-data">
                    <span class="log-data-title"># instances</span>: {{ log.JsonLogData['Count'] }}<br />
                    <span class="log-data-title">last upload</span>: {{ log.JsonLogData['Last'] }}
                </td>
                <td v-else-if="log.Action == 'start-anonymization-job'" class="log-data">
                    <span class="log-data-title">replaced</span>:<br />
                    <span v-for="(v, k) in log.JsonLogData['Payload']['Replace']" :key="k">{{ k + ": " + v
                        }}<br /></span>
                </td>
                <td v-else-if="log.Action == 'start-modification-job'" class="log-data">
                    <span class="log-data-title">replaced</span>:<br />
                    <span v-for="(v, k) in log.JsonLogData['Payload']['Replace']" :key="k">{{ k + ": " + v
                        }}<br /></span>
                    <span class="log-data-title">original-values</span>:<br />
                    <span v-for="(v, k) in log.JsonLogData['TagsBeforeModification']" :key="k">{{ k + ": " + v
                        }}<br /></span>
                </td>
                <td v-else class="log-data">{{ log.JsonLogData }}</td>
            </tr>
        </table>
    </div>
</template>

<style>
.audit-logs-table>:not(:first-child) {
    border-top: 0px !important;
}

.audit-logs-table>:first-child {
    border-bottom: 2px !important;
    border-style: solid !important;
    border-color: black !important;
}

.audit-logs-table {}

.audit-logs-table>:nth-child(odd)>* {
    background-color: var(--series-odd-bg-color);
}

.audit-logs-table>:nth-child(odd)>*>* {
    background-color: var(--series-odd-bg-color);
}

.audit-logs-table>:nth-child(odd)>*>*>* {
    background-color: var(--series-odd-bg-color);
}

.audit-logs-table>:nth-child(even)>* {
    background-color: var(--series-even-bg-color);
}

.audit-logs-table>:nth-child(even)>*>* {
    background-color: var(--series-even-bg-color);
}

.audit-logs-table>:nth-child(even)>*>*>* {
    background-color: var(--series-even-bg-color);
}

.audit-logs-table-header {
    text-align: left;
    padding-left: 10px;
}

.audit-logs-table td {
    text-align: left;
    padding-left: 10px;
}

.log-data {
    font-family: 'Courier New', Courier, monospace;
    font-size: small;
}

.log-data-title {
    font-weight: 700;
    text-decoration: underline;
}
</style>