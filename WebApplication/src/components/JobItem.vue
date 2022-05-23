<script>
import Modal from "./Modal.vue"
import { mapState } from "vuex"
import api from "../orthancApi"

export default {
    props: ["job"],
    emits: ["deletedJob"],
    data() {
        return {
            isComplete: false,
            isRunning: false,
            isSuccess: false,
            pctComplete: 0,
            pctFailed: 0,
            pctRemaining: 100,
            refreshTimeout: 200,
        };
    },
    mounted() {

    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
    },
    methods: {
        close(jobId) {
            this.$emit("deletedJob", jobId);
        },
        async refresh(jobId) {
            const jobStatus = await api.getJobStatus(jobId);
            this.isComplete = (jobStatus.State == "Success" || jobStatus.State == "Failure");
            this.isRunning = (jobStatus.State == "Running");
            this.isSuccess = (jobStatus.State == "Success");

            if (this.isComplete) {

                this.pctRemaining = 0;

                if (this.isSuccess) {
                    this.pctComplete = 100;
                    this.pctFailed = 0;
                } else {
                    this.pctComplete = 0;
                    this.pctFailed = 100;
                }
            } else {
                this.pctFailed = 0;
                this.pctComplete = jobStatus.Progress;
                this.pctRemaining = 100 - this.pctComplete;
            }
            if (!this.isComplete) {
                this.refreshTimeout = Math.min(this.refreshTimeout + 200, 2000);  // refresh quickly at the beginnning !
                setTimeout(this.refresh, this.refreshTimeout, [jobId]);
            }
        }
    },
    async mounted() {
        this.refresh(this.job['id']);
    },
    components: { Modal }
}
</script>

<template>
    <div class="card border-secondary job-card">
        <div class="card-header jobs-header">
            {{ job.name }}
            <button type="button" class="btn-close job-card-close" aria-label="Close" @click="close(job.id)"></button>
            <div class="progress" style="width:90%">
                <div class="progress-bar bg-success" role="progressbar"
                    v-bind:style="'width: ' + this.pctComplete + '%'"></div>
                <div class="progress-bar bg-secondary" role="progressbar"
                    v-bind:style="'width: ' + this.pctRemaining + '%'"></div>
                <div class="progress-bar bg-danger" role="progressbar"
                    v-bind:style="'width: ' + this.pctFailed + '%'"></div>
            </div>

        </div>
        <div class="card-body text-secondary jobs-body">
            <p class="card-text">
            </p>
        </div>

    </div>
</template>
<style scoped>
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
    color: black;
}

.job-card a {
    color: black;
}

.job-card-close {
    position: absolute;
    top: 5px;
    right: 5px;
}

.job-pct-pill {
    width: 50px;
    margin-right: 5px;
}

.upload-details li {
    border-left: 0px;
    text-align: left;
    border: 0px;
}
</style>