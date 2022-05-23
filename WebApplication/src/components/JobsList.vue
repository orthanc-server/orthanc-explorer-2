<script>
import JobItem from "./JobItem.vue"
import { mapState } from "vuex"

export default {
    props: [],
    data() {
        return {
        };
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            jobs: state => state.jobs.jobs,
            jobsIds: state => state.jobs.jobsIds
        }),
    },
    mounted() {
    },
    methods: {
        onDeletedJobItem(jobId) {
            this.$store.dispatch('jobs/removeJob', { jobId: jobId });
        },
        getJob(jobId) {
            return this.jobs[jobId];
        }
    },
    components: { JobItem }
}
</script>

<template>
    <div>
        <div class="upload-report-list">
            <JobItem
                v-for="jobId of jobsIds"
                :job="this.getJob(jobId)"
                :key="jobId"
                v-bind:href="jobId"
                @deletedJob="onDeletedJobItem"
            ></JobItem>
        </div>
    </div>
</template>
<style scoped>
.upload-report-list {
    display: flex;
    flex-direction: column-reverse;
}

.jobs {
    color: black;
}

.jobs-header {
    text-align: left;
    padding-top: 2px;
    padding-bottom: 2px;
}

.jobs-body {
    padding-top: 2px;
    padding-bottom: 2px;
    line-height: 1.6;
    text-align: left;
}
.jobs-body a {
    color: black;
    text-decoration: underline;
}

.upload-details-errors {
    text-align: left;
    border-bottom: 3px;
    border-top: 0px;
    border-left: 0px;
    border-right: 0px;
    border-color: gray;
    border-style: solid;
}
.job-card {
    margin-bottom: 2px;
}

.job-card-close {
    position: absolute;
    top: 5px;
    right: 5px;
}

.upload-details li {
    border-left: 0px;
    text-align: left;
    border: 0px;
}

.upload-handler-drop-zone {
    border-color: white;
    border-style: dashed;
    border-width: 4px;
}
</style>