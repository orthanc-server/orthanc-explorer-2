<script>
import api from "../orthancApi"
import { mapState } from "vuex"
import JobItemDetail from "./JobItemDetail.vue";
import JobItemResources from "./JobItemResources.vue";
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
            expanded: {},
            cancelRefreshAllJobs: null,
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
    },
    async mounted() {
        console.log("mounted");
        this.refreshAllJobs(true);
    },
    async unmounted() {
        console.log("unmounted");
        clearTimeout(this.cancelRefreshAllJobs);
        this.cancelRefreshAllJobs = null;
    },
    methods: {
        async refreshAllJobs(autoRefresh) {
            this.allJobs = await api.getAllJobs();
            this.expanded = {};

            this.allJobs.map(j => this.expanded[j.ID] = false);

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
            });

            if (autoRefresh) {
                this.cancelRefreshAllJobs = setTimeout(() => {this.refreshAllJobs(true);}, 5000);
            }
        },
        isSuccess(job) {
            return (job.State == "Success");
        },
        async refreshJob(job) {
            const updatedJob = await api.getJobStatus(job.ID);
            const i = this.allJobs.findIndex(j => j.ID === j.ID);
            if (i !== -1) {
                this.allJobs[i] = updatedJob;
            }
        },
        isComplete(job) {
            return (job.State == "Success" || job.State == "Failure");
        },
        isRunning(job) {
            return (job.State == "Running");
        },
        isExpanded(job) {
            return this.expanded[job.ID];
        },
        toggleExpanded(job) {
            this.expanded[job.ID] = !this.expanded[job.ID];
        },
        getStatusText(job) {
            if (this.isSuccess(job)) {
                return this.$t('job_success');
            } else if (this.isComplete(job)) {
                return this.$t('job_failure');
            }
            return job.Progress + " %";
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
            if (job.State == 'Running') {
                return job.Timestamp;
            } else {
                return job.CompletionTime || job.CreationTime;
            }
        },
        async pause(jobId) {
            await api.pauseJob(jobId);
            await this.refreshAllJobs(false);
        },
        async cancel(jobId) {
            await api.cancelJob(jobId);
            await this.refreshAllJobs(false);
        },
        async resume(jobId) {
            await api.resumeJob(jobId);
            await this.refreshAllJobs(false);
        },
        async resubmit(jobId) {
            await api.resubmitJob(jobId);
            await this.refreshAllJobs(false);
        },
        async deleteJob(jobId) {
            await api.deleteJob(jobId);
            await this.refreshAllJobs(false);
        }
    },
    components: { JobItemDetail, JobItemResources }
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
                                <div v-if="isRunning(job) || isComplete(job)" class="progress-bar bg-success" role="progressbar"
                                    v-bind:style="'width: ' + getPctComplete(job) + '%'">{{ getStatusText(job) }}</div>
                                <div v-if="job.State=='Pending' || job.State=='Paused'" class="progress-bar bg-info" role="progressbar"
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
                        <td v-if="job.Type == 'DicomModalityStore'" colspan="2" class="job-details">
                            <JobItemDetail title="Remote AET" :value="job.Content.RemoteAet"></JobItemDetail>
                            <JobItemDetail title="Local AET" :value="job.Content.LocalAet"></JobItemDetail>
                            <JobItemDetail title="Instances Count" :value="job.Content.InstancesCount"></JobItemDetail>
                            <JobItemDetail title="Error details" :value="job.ErrorDetails" isError="true"></JobItemDetail>
                            <JobItemResources :resources="job.Content.Resources"></JobItemResources>
                        </td>
                        <td v-else-if="job.Type == 'OrthancPeerStore'" colspan="2" class="job-details">
                            <JobItemDetail title="Peer" :value="job.Content.Peer"></JobItemDetail>
                            <JobItemDetail title="Instances Count" :value="job.Content.InstancesCount"></JobItemDetail>
                            <JobItemDetail title="Error details" :value="job.ErrorDetails" isError="true"></JobItemDetail>
                            <JobItemResources :resources="job.Content.Resources"></JobItemResources>
                        </td>
                        <td v-else-if="job.Type == 'ResourceModification'" colspan="2" class="text-start job-details">
                            <JobItemDetail title="Is Anonymization" :value="job.Content.IsAnonymization"></JobItemDetail>
                            <JobItemDetail title="Error details" :value="job.ErrorDetails" isError="true"></JobItemDetail>
                            <JobItemResources :resources="job.Content.Resources"></JobItemResources>
                        </td>
                        <td v-else colspan="2">
                            <JobItemDetail title="Error details" :value="job.ErrorDetails" isError="true"></JobItemDetail>
                            <JobItemResources v-if="job.Content && job.Content.Resources" :resources="job.Content.Resources"></JobItemResources>
                        </td>
                        <td></td>
                        <td colspan="2" class="text-start job-full-details">
                            <button v-if="job.State=='Failure'" class="btn btn-primary btn-sm mx-1" type="button" @click="resubmit(job.ID)">
                                {{ $t('jobs.button_title_resubmit') }}
                            </button>
                            <button v-if="job.State=='Failure' || job.State=='Success'" class="btn btn-primary btn-sm mx-1" type="button" @click="deleteJob(job.ID)">
                                {{ $t('jobs.button_title_delete') }}
                            </button>
                            <button v-if="job.State=='Running'" class="btn btn-primary btn-sm mx-1" type="button" @click="pause(job.ID)">
                                {{ $t('jobs.button_title_pause') }}
                            </button>
                            <button v-if="job.State=='Running' || job.State=='Pending' || job.State=='Paused'" class="btn btn-primary btn-sm mx-1" type="button" @click="cancel(job.ID)">
                                {{ $t('jobs.button_title_cancel') }}
                            </button>
                            <button v-if="job.State=='Paused'" class="btn btn-primary btn-sm mx-1" type="button" @click="resume(job.ID)">
                                {{ $t('jobs.button_title_resume') }}
                            </button>
                            <button class="btn btn-secondary btn-sm" type="button" data-bs-toggle="collapse" :data-bs-target="'#job-collapse-details-' + job.ID">
                                {{ $t('jobs.button_title_full_details') }}
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