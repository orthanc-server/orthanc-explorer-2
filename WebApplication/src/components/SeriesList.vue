<script>
import axios from "axios"
import api from "../orthancApi"
import SeriesItem from "./SeriesItem.vue"
import { translateDicomTag } from "../locales/i18n"

export default {
    props: ['studyId', 'patientMainDicomTags', 'studyMainDicomTags'],
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
        columnTitle(tagName) {
            if (tagName == "instances_number") {
                return "# " + this.$i18n.t('instances');
            } else {
                return translateDicomTag(this.$i18n.t, this.$i18n.te, tagName);
            }
        },
        columnTooltip(tagName) {
            if (tagName == "instances_number") {
                return this.$i18n.t("instances_number");
            } else {
                return translateDicomTag(this.$i18n.t, this.$i18n.te, tagName);
            }
        },
        onDeletedSeries(seriesId) {
            delete this.seriesInfo[seriesId];
            if (Object.keys(this.seriesInfo).length == 0) {
                this.$emit("deletedStudy", this.studyId);
            }
        }
    },
    async mounted() {
        const studyResponse = await api.getStudySeries(this.studyId);

        for (const series of studyResponse) {
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
                :title="columnTooltip('SeriesNumber')"
                >{{columnTitle('SeriesNumber')}}</th>
                <th
                width="40%"
                scope="col"
                class="series-table-header cut-text"
                data-bs-toggle="tooltip"
                :title="columnTooltip('SeriesDescription')"
                >{{columnTitle('SeriesDescription')}}</th>
                <th
                width="11%"
                scope="col"
                class="series-table-header cut-text text-center"
                data-bs-toggle="tooltip"
                :title="columnTooltip('Modality')"
                >{{columnTitle('Modality')}}</th>
                <th
                width="5%"
                scope="col"
                class="series-table-header cut-text text-center"
                data-bs-toggle="tooltip"
                :title="columnTooltip('instances_number')"
                >{{columnTitle('instances_number')}}</th>
        </thead>
        <SeriesItem
            v-for="seriesId in sortedSeriesIds"
            :key="seriesId"
            :seriesId="seriesId"
            :seriesInfo="seriesInfo[seriesId]"
            :studyMainDicomTags="this.studyMainDicomTags"
            :patientMainDicomTags="this.patientMainDicomTags"
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

.series-table>:nth-child(odd) {
    background-color: var(--series-odd-color);
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