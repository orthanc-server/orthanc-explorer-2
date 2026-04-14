<script>
import api from "../orthancApi"
import { mapState } from "vuex"
import JobItemDetail from "./JobItemDetail.vue";
import dateHelpers from "../helpers/date-helpers"

const sortedStates = {
    "Pending": 10, 
    "Retry": 9, 
    "Paused": 9,
    "Running": 8, 
    "Success": 7, 
    "Failure": 7 
};

export default {
    props: [],
    data() {
        return {
            allJobs: [],
            allJobsSorted: [],
        };
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            isConfigurationLoaded: state => state.configuration.loaded,
        }),
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
        }
    },
    async mounted() {
        this.updateFromRoute(this.$route.query);
    },
    methods: {
        async updateFromRoute(filters) {
            this.allJobs = await api.getAllJobs();

            this.allJobs.map(j => j.Expanded = false);
            this.allJobs.sort((a, b) => {
                // first order by State
                if (sortedStates[a.State] != sortedStates[b.State]) {
                    return sortedStates[b.State] - sortedStates[a.State];
                }
                
                // then, order pending jobs by priority
                if (a.State == 'Pending' && a.Priority != b.Priority) {
                    return b.Priority - a.Priority;
                } 

                // then, order by date
                const parseCustomDate = (dateStr) => { // Helper function to parse the custom date format (20260320T111858.280279)
                    return {
                        'date': dateStr.substring(0, 8),  // '20260320'
                        'time': dateStr.substring(9, 15), // '111858'
                        'microsec': dateStr.substring(16, 22) // '280279'
                    }
                };
                const timeA = parseCustomDate(a.CompletionTime || a.CreationTime);
                const timeB = parseCustomDate(b.CompletionTime || b.CreationTime);

                if (timeA.date != timeB.date) {
                    return (timeA.date > timeB.date ? -1 : 0);
                }

                if (timeA.time != timeB.time) {
                    return (timeA.time > timeB.time ? -1 : 0);
                }

                if (timeA.microsec != timeB.microsec) {
                    return (timeA.microsec > timeB.microsec ? -1 : 0);
                }
                return 0;
            })
        },
        isSuccess(job) {
            return (job.State == "Success");
        },
        isComplete(job) {
            return (job.State == "Success" || job.State == "Failure");
        },
        isRunning(job) {
            return (job.State == "Running");
        },
        isExpanded(job) {
            return job.Expanded;
        },
        toggleExpanded(job) {
            job.Expanded = !job.Expanded;
        },
        getStatusText(job) {
            if (this.isSuccess(job)) {
                return this.$t('job_success');
            } else if (this.isComplete(job)) {
                return this.$t('job_failure');
            }
            return "";
        },
        getPctComplete(job) {
            if (this.isComplete(job)) {
                if (this.isSuccess(job)) {
                    return 100;
                } else {
                    return 0;
                }
            } else {
                return job.Progress;
            }
        },
        getPctRemaining(job) {
            if (this.isComplete(job)) {
                return 0;
            } else {
                return 100 - this.getPctComplete(job);
            }
        },
        getPctFailed(job) {
            if (this.isComplete(job)) {
                if (this.isSuccess(job)) {
                    return 0;
                } else {
                    return 100;
                }
            } else {
                return 0;
            }
        },
        getJobTime(job) {
            return job.CompletionTime || job.CreationTime;
        }
    },
    components: { JobItemDetail }
}
</script>

<template>
    <div>
        <table class="table table-responsive table-sm job-table">
            <thead>
                <tr>
                    <th width="5%" scope="col" class="audit-logs-table-header cut-text text-start"
                        data-bs-toggle="tooltip" :title="$t('jobs.column_title_id')">{{
                            $t('jobs.column_title_id') }}</th>
                    <th width="15%" scope="col" class="audit-logs-table-header cut-text text-start"
                        data-bs-toggle="tooltip" :title="$t('jobs.column_title_type')">
                        {{ $t('jobs.column_title_type') }}</th>
                    <th width="12%" scope="col" class="audit-logs-table-header cut-text text-start"
                        data-bs-toggle="tooltip" :title="$t('jobs.column_title_status')">
                        {{ $t('jobs.column_title_status') }}</th>
                    <th width="5%" scope="col" class="audit-logs-table-header cut-text text-start"
                        data-bs-toggle="tooltip" :title="$t('jobs.column_title_status')">
                        {{ $t('jobs.column_title_priority') }}</th>
                    <th width="15%" scope="col" class="audit-logs-table-header cut-text text-start"
                        data-bs-toggle="tooltip" :title="$t('jobs.column_title_last_update')">
                        {{ $t('jobs.column_title_progress') }}</th>
                    <th width="15%" scope="col" class="audit-logs-table-header cut-text text-start"
                        data-bs-toggle="tooltip" :title="$t('jobs.column_title_last_update')">
                        {{ $t('jobs.column_title_last_update') }}</th>
                </tr>
            </thead>
            <tbody>
                <template v-for="job in allJobs" :key="job.ID">
                    <tr data-bs-toggle="collapse" :href="'#job-collapse-' + job.ID"
                        class="job-row">
                        <td class="cut-text text-start">{{ job.ID }}</td>
                        <td class="cut-text text-start">{{ job.Type }}</td>
                        <td class="cut-text text-start">{{ job.State }}</td>
                        <td class="cut-text text-start">{{ job.Priority }}</td>
                        <td class="cut-text">
                            <div class="progress mt-1 mb-1" style="width:90%">
                                <div class="progress-bar bg-success" role="progressbar"
                                    v-bind:style="'width: ' + getPctComplete(job) + '%'">{{ getStatusText(job) }}</div>
                                <div class="progress-bar bg-secondary" role="progressbar"
                                    v-bind:style="'width: ' + getPctRemaining(job) + '%'"></div>
                                <div class="progress-bar bg-danger" role="progressbar" v-bind:style="'width: ' + getPctFailed(job) + '%'">
                                    {{ getStatusText(job) }}</div>
                            </div>
                        </td>
                        <td class="cut-text text-start">{{ getJobTime(job) }}</td>
                    </tr>
                    <tr class="collapse" :id="'job-collapse-' + job.ID">
                        <td></td>
                        <td v-if="job.Type == 'DicomModalityStore'" colspan="2" class="text-start job-details">
                            <JobItemDetail title="Remote AET" :value="job.Content.RemoteAet"></JobItemDetail>
                            <JobItemDetail title="Local AET" :value="job.Content.LocalAet"></JobItemDetail>
                            <JobItemDetail title="Instances Count" :value="job.Content.InstancesCount"></JobItemDetail>
                        </td>
                        <td v-else-if="job.Type == 'ResourceModifiction'" colspan="2" class="text-start job-details">
                            <JobItemDetail title="Is Anonymization" :value="job.Content.IsAnonymization"></JobItemDetail>
                        </td>
                        <td v-else colspan="2">
                        </td>
                        <td></td>
                        <td colspan="2" class="text-start job-full-details">
                            <template v-if="job.ErrorDetails"><strong>Error details:</strong> {{ job.ErrorDetails }}<br/></template>
                            <button class="btn btn-primary btn-sm" type="button" data-bs-toggle="collapse" :data-bs-target="'#job-collapse-details-' + job.ID">
                                {{ $t('jobs.full_details_button_title') }}
                            </button>
                            <pre class="collapse job-full-json" :id="'job-collapse-details-' + job.ID">{{ JSON.stringify(job, null, 2) }}
                            </pre>
                        </td>
                    </tr>
                </template>
            </tbody>
        </table>
    </div>
</template>

<style>
.job-table>:not(caption)>*>* {
    background-color: var(--study-bg-color);
}

.job-table>tbody>tr.job-row {
    background-color: var(--study-odd-bg-color);
}

.job-table>tbody>tr.job-row:hover {
    background-color: var(--study-hover-color) !important;
    cursor: pointer;
}

.job-details {
    font-size: smaller;
}

.job-full-details {
    font-size: smaller;
    max-width: 40vw;
}

.job-full-json {
    font-size: smaller;
    overflow-y: auto;
    max-height: 80vh;
    white-space: pre-wrap;       /* Preserve whitespace but allow wrapping */
    word-wrap: break-word;      /* Break long words if necessary */
}
</style>