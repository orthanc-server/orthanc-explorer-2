<script>
import JobSideItem from "./JobSideItem.vue"
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
    components: { JobSideItem }
}
</script>

<template>
    <div>
        <div class="upload-report-list">
            <JobSideItem v-for="jobId of jobsIds" :job="this.getJob(jobId)" :key="jobId" v-bind:href="jobId"
                @deletedJob="onDeletedJobItem"></JobSideItem>
        </div>
    </div>
</template>
<style scoped></style>