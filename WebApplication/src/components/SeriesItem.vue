<script>
import SeriesDetails from "./SeriesDetails.vue";
import { mapState, mapGetters } from "vuex"
import SourceType from '../helpers/source-type';
import SelectionStatus from "../helpers/selection-status";
import { useIsMobileAny, useIsMobileNarrow, useIsMobileWide } from "../helpers/media-helpers";

const expandedSeries = new Set();

export default {
    props: ["seriesId", "seriesInfo", "studySeries", 'studyMainDicomTags', 'patientMainDicomTags'],
    emits: ['deletedSeries'],
    data() {
        return {
            loaded: false,
            expanded: expandedSeries.has(this.seriesId),
            isMobileAny: useIsMobileAny(),
            isMobileNarrow: useIsMobileNarrow(),
            isMobileWide: useIsMobileWide(),
        };
    },
    computed: {
        ...mapState({
            studiesSourceType: state => state.studies.sourceType,
        }),
        isTableMode() {
            return !this.isMobileAny || this.isMobileWide;
        },
        isMultipleSelectionEnabled() {
            return this.studiesSourceType != SourceType.REMOTE_DICOM;
        },
        instancesCount() {
            if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
                return this.seriesInfo.Instances.length;
            } else if (this.studiesSourceType == SourceType.REMOTE_DICOM || this.studiesSourceType == SourceType.REMOTE_DICOM_WEB) {
                return this.seriesInfo.MainDicomTags.NumberOfSeriesRelatedInstances;
            }
        },
        parentStudyId() {
            if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
                return this.seriesInfo['ParentStudy'];
            } else if (this.studiesSourceType == SourceType.REMOTE_DICOM || this.studiesSourceType == SourceType.REMOTE_DICOM_WEB) {
                return this.seriesInfo['MainDicomTags']['StudyInstanceUID'];
            }
        },
        isSelected() {
            return this.$store.getters['selection/getSeriesSelectionStatus'](this.parentStudyId, this.seriesId) != SelectionStatus.NOT_SELECTED;
        },
        isPartialySelected() {
            return this.$store.getters['selection/getSeriesSelectionStatus'](this.parentStudyId, this.seriesId) == SelectionStatus.PARTIAL;
        }

    },
    mounted() {
        for (const [k, v] of Object.entries(this.$route.query)) {
            if (k === 'expand') {
                if (['series', 'instance'].indexOf(v) != -1) {
                    this.expanded = true;
                }
            }
        }
    },
    methods: {
        onDeletedSeries() {
            this.$emit("deletedSeries", this.seriesId);
        },
        async clickedSelect() {
            await this.$store.dispatch('selection/selectSeries', { studyId: this.parentStudyId, seriesId: this.seriesId, studySeries: this.studySeries, isSelected: !this.isSelected }); // this.selected is the value before the click
        }
    },
    watch: {
        // keep the expanded state in a module-level set so it survives
        // the unmount/remount that happens when switching view mode
        expanded(value) {
            if (value) {
                expandedSeries.add(this.seriesId);
            } else {
                expandedSeries.delete(this.seriesId);
            }
        }
    },
    components: { SeriesDetails }
}
</script>


<template>
    <div style="display: contents">
        <tbody v-if="isTableMode">
            <tr :class="{ 'series-row-collapsed': !expanded, 'series-row-expanded': expanded }">
                <td>
                    <div v-if="isMultipleSelectionEnabled" class="form-check">
                        <input class="form-check-input" type="checkbox" :checked="isSelected"
                            :indeterminate="isPartialySelected" @click="clickedSelect">
                    </div>
                </td>
                <td class="cut-text text-center" @click="expanded = !expanded">{{ seriesInfo.MainDicomTags.SeriesNumber
                    }}
                </td>
                <td class="cut-text" @click="expanded = !expanded">
                    <span data-bs-toggle="tooltip" v-bind:title="seriesInfo.MainDicomTags.SeriesDescription">{{
                        seriesInfo.MainDicomTags.SeriesDescription }}</span>
                </td>
                <td class="cut-text text-center" @click="expanded = !expanded">{{ seriesInfo.MainDicomTags.Modality }}
                </td>
                <td class="cut-text text-center" @click="expanded = !expanded">{{ instancesCount }}</td>
            </tr>
            <tr v-if="expanded" class="series-details-expanded">
                <td colspan="100">
                    <SeriesDetails v-if="this.expanded" :seriesId="this.seriesId"
                        :instancesIds="this.seriesInfo.Instances" :studySeries="this.studySeries"
                        :studyId="this.parentStudyId" :seriesMainDicomTags="this.seriesInfo.MainDicomTags"
                        :studyMainDicomTags="this.studyMainDicomTags" :patientMainDicomTags="this.patientMainDicomTags"
                        @deletedSeries="onDeletedSeries"></SeriesDetails>
                </td>
            </tr>
        </tbody>
        <div v-else-if="isMobileNarrow" class="mobile-series-card d-flex flex-column gap-1"
            :class="{ 'expanded': expanded, 'selected': isSelected }">
            <div class="resource-card card mb-2" @click="expanded = !expanded" style="cursor: pointer">
                <div class="card-body p-0">
                    <div class="d-flex">
                        <div
                            class="resource-card-icons-small d-flex flex-row justify-content-around align-items-center py-2">
                            <input class="form-check-input" type="checkbox" :checked="isSelected"
                                :indeterminate="isPartialySelected" @click.stop="clickedSelect">
                        </div>
                        <div class="resource-card-info flex-grow-1">
                            <div class="resource-card-line-1">
                                <div class="field-w-20">{{ seriesInfo.MainDicomTags.SeriesNumber }}</div>
                                <div class="field-w-50">{{ seriesInfo.MainDicomTags.SeriesDescription }}</div>
                                <div class="field-w-20">{{ seriesInfo.MainDicomTags.Modality }}</div>
                                <div class="field-w-10 text-end">{{ instancesCount }}</div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
            <div v-if="expanded" class="border-top p-2">
                <SeriesDetails v-if="this.expanded" :seriesId="this.seriesId"
                    :instancesIds="this.seriesInfo.Instances" :studySeries="this.studySeries"
                    :studyId="this.parentStudyId" :seriesMainDicomTags="this.seriesInfo.MainDicomTags"
                    :studyMainDicomTags="this.studyMainDicomTags" :patientMainDicomTags="this.patientMainDicomTags"
                    @deletedSeries="onDeletedSeries"></SeriesDetails>
            </div>
        </div>
    </div>
</template>

<style scoped>
.series-row-expanded {
    background-color: var(--series-selected-color);
    font-weight: 700;

    border-top: 3px !important;
    border-style: solid !important;
    border-color: black !important;
}

.series-row-expanded>:first-child {
    border-bottom: 0px !important;
}

.series-details-expanded {
    background-color: var(--series-details-bg-color);

    border-top: 0px !important;
    border-bottom: 3px !important;
    border-style: solid !important;
    border-color: black !important;
}
</style>