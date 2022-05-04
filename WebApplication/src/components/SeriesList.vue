<script>
import axios from "axios"
import api from "../orthancApi"
import SeriesItem from "./SeriesItem.vue"

export default {
    props: ['studyId'],
    emits: ['deletedStudy'],
    data() {
        return {
            seriesInfo: {},
        };
    },
    computed: {
        sortedSeriesIds() {
            let keys = Object.keys(this.seriesInfo);
            keys.sort((a, b) => (parseInt(this.seriesInfo[a].MainDicomTags.SeriesNumber) > parseInt(this.seriesInfo[b].MainDicomTags.SeriesNumber) ? 1 : -1))
            return keys;
        }
    },
    methods: {
        onDeletedSeries(seriesId) {
            delete this.seriesInfo[seriesId];
            if (Object.keys(this.seriesInfo).length == 0) {
                this.$emit("deletedStudy", this.studyId);
            }
        }
    },
    async mounted() {
        const studyResponse = await api.getStudySeries(this.studyId);

        for (const series of studyResponse.data) {
            this.seriesInfo[series["ID"]] = series;
        }

    },
    components: { SeriesItem }
}
</script>

<template>
    <table class="table table-responsive table-sm series-table">
        <thead>
            <th width="2%" scope="col" class="series-table-header"></th>
            <th
                width="7%"
                scope="col"
                class="series-table-header cut-text"
                data-bs-toggle="tooltip"
                title="Series Number"
            >Series Number</th>
            <th
                width="40%"
                scope="col"
                class="series-table-header cut-text"
                data-bs-toggle="tooltip"
                title="Series Description"
            >Series Description</th>
            <th
                width="11%"
                scope="col"
                class="series-table-header cut-text text-center"
                data-bs-toggle="tooltip"
                title="Modality"
            >Modality</th>
            <th
                width="5%"
                scope="col"
                class="series-table-header cut-text text-center"
                data-bs-toggle="tooltip"
                title="Number of Instances"
            ># instances</th>
        </thead>
        <SeriesItem
            v-for="seriesId in sortedSeriesIds"
            :key="seriesId"
            :seriesId="seriesId"
            :seriesInfo="seriesInfo[seriesId]"
            @deletedSeries="onDeletedSeries"
        ></SeriesItem>
    </table>
</template>

<style>

.series-table>:not(:first-child) {
    border-top: 0px !important;
}

.series-table>:first-child {
    border-bottom: 2px !important;
    border-style: solid !important;
    border-color: black !important;
}

.series-table-header {
    text-align: left;
    padding-left: 10px;
}

.series-table td {
    text-align: left;
    padding-left: 10px;
}

</style>