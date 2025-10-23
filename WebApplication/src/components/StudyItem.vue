<script>
import SeriesList from "./SeriesList.vue"
import StudyDetails from "./StudyDetails.vue";
import { mapState } from "vuex"
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js"
import api from "../orthancApi";
import dateHelpers from "../helpers/date-helpers"
import SourceType from '../helpers/source-type';
import resourceHelpers from "../helpers/resource-helpers";
import TokenLinkButton from "./TokenLinkButton.vue";

export default {
    props: ["studyId"],
    emits: ["deletedStudy"],
    data() {
        return {
            study: {},
            loaded: false,
            expanded: false,
            collapseElement: null,
            selected: false,
            pdfReports: []
        };
    },
    created() {
        this.messageBus.on('selected-all', this.onSelectedStudy);
        this.messageBus.on('unselected-all', this.onUnselectedStudy);
        this.messageBus.on('added-series-to-study-' + this.studyId, () => {this.reloadStudy(this.studyId)});
        this.messageBus.on('deleted-series-from-study-' + this.studyId, () => {this.reloadStudy(this.studyId)});
        this.messageBus.on('study-updated-' + this.studyId, () => {this.reloadStudy(this.studyId)});
        this.messageBus.on('study-labels-updated-in-labels-editor-' + this.studyId, () => {this.reloadStudy(this.studyId)});
    },
    async mounted() {
        this.study = this.studies.filter(s => s["ID"] == this.studyId)[0];
        this.loaded = true;
        this.seriesIds = this.study.Series;
        this.selected = this.selectedStudiesIds.indexOf(this.studyId) != -1;

        if (!this.$refs['study-collapsible-details']) {
            console.log('no refs: ', studyResponse);
        }
        this.$refs['study-collapsible-details'].addEventListener('show.bs.collapse', (e) => {
            if (e.target == e.currentTarget) {
                this.expanded = true;
            }
        });
        this.$refs['study-collapsible-details'].addEventListener('hide.bs.collapse', (e) => {
            if (e.target == e.currentTarget) {
                this.expanded = false;
            }
        });

        var el = this.$refs['study-collapsible-details'];
        this.collapseElement = new bootstrap.Collapse(el, { toggle: false });

        for (const [k, v] of Object.entries(this.$route.query)) {
            if (k === 'expand') {
                if (v == null || v === 'study' || v === 'series' || v === 'instance') {
                    this.collapseElement.show();
                }
            }
        }

        if (this.hasPdfReportIcon) {
            let instances = await api.getStudyInstancesExpanded(this.study.ID, ["SOPClassUID", "SeriesDate", "SeriesDescription"]);
            for (let instance of instances) {
                if (instance.RequestedTags.SOPClassUID == "1.2.840.10008.5.1.4.1.1.104.1") {
                    let titles = [];
                    if (instance.RequestedTags.SeriesDate) {
                        titles.push(dateHelpers.formatDateForDisplay(instance.RequestedTags.SeriesDate, this.uiOptions.DateFormat));
                    }
                    if (instance.RequestedTags.SeriesDescription) {
                        titles.push(instance.RequestedTags.SeriesDescription);
                    }

                    this.pdfReports.push({
                        'url': api.getInstancePdfUrl(instance.ID),
                        'title': titles.join(' - '),
                        'id': instance.ID
                    });
                }
            }
        }

    },
    methods: {
        onDeletedStudy(studyId) {
            this.$emit("deletedStudy", this.studyId);
        },
        async reloadStudy(studyId) {
            // console.log("StudyItem: reloadStudy", studyId);
            await this.$store.dispatch('studies/reloadStudy', {
                'studyId': studyId,
                'study': await api.getStudy(studyId)
            })
            this.study = this.studies.filter(s => s["ID"] == this.studyId)[0];
            // console.log("StudyItem: study reloaded ", this.study.Labels);
        },
        onSelectedStudy() {
            this.selected = true;
        },
        onUnselectedStudy() {
            this.selected = false;
        },
        async clickedSelect() {
            // console.log(this.studyId, this.selected);
            await this.$store.dispatch('studies/selectStudy', { studyId: this.studyId, isSelected: !this.selected }); // this.selected is the value before the click
            this.selected = !this.selected;
            // console.log(this.studyId, this.selected);
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            studies: state => state.studies.studies,
            studiesSourceType: state => state.studies.sourceType,
            selectedStudiesIds: state => state.studies.selectedStudiesIds,
            allLabels: state => state.labels.allLabels
        }),
        modalitiesInStudyForDisplay() {
            if (this.study.RequestedTags.ModalitiesInStudy) {
                return this.study.RequestedTags.ModalitiesInStudy.split('\\').join(',');
            } else {
                return "";
            }
        },
        showLabels() {
            if (this.studiesSourceType == SourceType.LOCAL_ORTHANC) {
                return !this.expanded && ((this.allLabels && this.allLabels.length > 0));
            } else {
                return false;
            }
        },
        hasLabels() {
            return this.study && this.study.Labels && this.study.Labels.length > 0;
        },
        formattedPatientName() {
            return resourceHelpers.formatPatientName(this.study.PatientMainDicomTags.PatientName);
        },
        formattedPatientBirthDate() {
            return dateHelpers.formatDateForDisplay(this.study.PatientMainDicomTags.PatientBirthDate, this.uiOptions.DateFormat);
        },
        formattedStudyDate() {
            return dateHelpers.formatDateForDisplay(this.study.MainDicomTags.StudyDate, this.uiOptions.DateFormat);
        },
        seriesCount() {
            if (this.study.sourceType == SourceType.REMOTE_DICOM || this.study.sourceType == SourceType.REMOTE_DICOM_WEB) {
                return this.study.MainDicomTags.NumberOfStudyRelatedSeries;
            } else if (this.study.Series) {
                return this.study.Series.length;
            }
        },
        instancesCount() {
            if (this.study.RequestedTags) {
                return this.study.RequestedTags.NumberOfStudyRelatedInstances;
            }
        },
        seriesAndInstancesCount() {
            const seriesCount = this.seriesCount;
            const instancesCount = this.instancesCount;
            if (instancesCount) {
                return String(seriesCount) + "/" + String(instancesCount);
            } else {
                return seriesCount;
            }
        },
        hasPdfReportIconColumn() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.uiOptions.EnableReportQuickButton;
        },
        hasPdfReportIconPlaceholder() {
            return this.hasPdfReportIconColumn && !this.hasPdfReportIcon;
        },
        hasPdfReportIcon() {
            return this.study.RequestedTags.SOPClassesInStudy && this.study.RequestedTags.SOPClassesInStudy.indexOf("1.2.840.10008.5.1.4.1.1.104.1") != -1 && this.uiOptions.EnableReportQuickButton;
        },
        hasPrimaryViewerColumn() {
            return this.studiesSourceType == SourceType.LOCAL_ORTHANC && this.uiOptions.EnableViewerQuickButton;
        },
        hasPrimaryViewerIconPlaceholder() {
            return this.hasPrimaryViewerColumn && !this.hasPrimaryViewerIcon;
        },
        hasPrimaryViewerIcon() {
            return this.hasPrimaryViewerColumn && this.primaryViewerUrl;
        },
        primaryViewerUrl() {
            return resourceHelpers.getPrimaryViewerUrl("study", this.study.ID, this.study.MainDicomTags.StudyInstanceUID);
        },
        primaryViewerTokenType() {
            return resourceHelpers.getPrimaryViewerTokenType();
        },
        colSpanStudyDetails() {
            let span = this.uiOptions.StudyListColumns.length + 1; // +1 for the 'checkbox'
            if (this.hasPrimaryViewerColumn) {
                span++;
            }
            if (this.hasPdfReportIconColumn) {
                span++;
            }
            return span;
        }
    },
    components: { SeriesList, StudyDetails, TokenLinkButton }
}
</script>


<template>
    <tbody>
        <tr v-if="loaded" :class="{ 'study-row-collapsed': !expanded, 'study-row-expanded': expanded, 'study-row-show-labels': showLabels }">
            <td>
                <div class="form-check">
                    <input class="form-check-input" type="checkbox" v-model="selected" @click="clickedSelect">
                </div>
            </td>
            <td v-if="hasPrimaryViewerIcon" class="td-viewer-icon">
                <TokenLinkButton v-if="primaryViewerUrl"
                    level="study" :linkUrl="primaryViewerUrl"
                    :resourcesOrthancId="[study.ID]" linkType="icon"
                    iconClass="bi bi-eye-fill"
                    :tokenType="primaryViewerTokenType" :opensInNewTab="true">
                </TokenLinkButton>
            </td>
            <td v-if="hasPrimaryViewerIconPlaceholder"></td>
            <td v-if="hasPdfReportIcon" class="td-pdf-icon">
                <TokenLinkButton v-for="pdfReport in pdfReports" :key="pdfReport.id"
                    level="study" :linkUrl="pdfReport.url"
                    :resourcesOrthancId="[study.ID]" linkType="icon"
                    iconClass="bi bi-file-earmark-text"
                    :tokenType="'download-instant-link'" :opensInNewTab="true"
                    :title="pdfReport.title">
                </TokenLinkButton>
            </td>
            <td v-if="hasPdfReportIconPlaceholder"></td>

            <td v-for="columnTag in uiOptions.StudyListColumns" :key="columnTag" class="cut-text"
                :class="{ 'text-center': columnTag in ['modalities', 'seriesCount', 'instancesCount', 'seriesAndInstancesCount'] }" data-bs-toggle="collapse"
                v-bind:data-bs-target="'#study-details-' + this.studyId">
                <span v-if="columnTag == 'StudyDate'" data-bs-toggle="tooltip"
                    v-bind:title="formattedStudyDate">{{ formattedStudyDate }}
                </span>
                <span v-else-if="columnTag == 'AccessionNumber'" data-bs-toggle="tooltip"
                    v-bind:title="study.MainDicomTags.AccessionNumber">{{ study.MainDicomTags.AccessionNumber }}
                </span>
                <span v-else-if="columnTag == 'PatientID'" data-bs-toggle="tooltip"
                    v-bind:title="study.PatientMainDicomTags.PatientID">{{ study.PatientMainDicomTags.PatientID }}
                </span>
                <span v-else-if="columnTag == 'PatientSex'" data-bs-toggle="tooltip"
                    v-bind:title="study.PatientMainDicomTags.PatientSex">{{ study.PatientMainDicomTags.PatientSex }}
                </span>
                <span v-else-if="columnTag == 'PatientName'" data-bs-toggle="tooltip"
                    v-bind:title="formattedPatientName">{{ formattedPatientName }}
                </span>
                <span v-else-if="columnTag == 'PatientBirthDate'" data-bs-toggle="tooltip"
                    v-bind:title="formattedPatientBirthDate">{{ formattedPatientBirthDate }}
                </span>
                <span v-else-if="columnTag == 'StudyDescription'" data-bs-toggle="tooltip"
                    v-bind:title="study.MainDicomTags.StudyDescription">{{ study.MainDicomTags.StudyDescription }}
                </span>
                <span v-else-if="columnTag == 'modalities'" data-bs-toggle="tooltip"
                    v-bind:title="modalitiesInStudyForDisplay">{{
                        modalitiesInStudyForDisplay }}
                </span>
                <span v-else-if="columnTag == 'seriesCount'" data-bs-toggle="tooltip"
                v-bind:title="seriesCount">{{ seriesCount }}
                </span>
                <span v-else-if="columnTag == 'instancesCount'" data-bs-toggle="tooltip"
                v-bind:title="instancesCount">{{ instancesCount }}
                </span>
                <span v-else-if="columnTag == 'seriesAndInstancesCount'" data-bs-toggle="tooltip"
                v-bind:title="seriesAndInstancesCount">{{ seriesAndInstancesCount }}
                </span>
                <span v-else>{{ study.MainDicomTags[columnTag] }}
                </span>
            </td>
        </tr>
        <tr v-show="showLabels">
            <td></td>
            <td v-if="hasPrimaryViewerColumn"></td>
            <td v-if="hasPdfReportIconColumn"></td>
            <td :colspan="uiOptions.StudyListColumns.length" class="label-row">
                <span v-for="label in study.Labels" :key="label" class="label badge">{{ label }}</span>
                <span v-if="!hasLabels">&nbsp;</span>
            </td>
        </tr>
        <tr v-show="loaded" class="collapse"
            :class="{ 'study-details-collapsed': !expanded, 'study-details-expanded': expanded }"
            v-bind:id="'study-details-' + this.studyId" ref="study-collapsible-details">
            <td v-if="loaded && expanded" :colspan="colSpanStudyDetails">
                <StudyDetails :studyId="this.studyId" :studyMainDicomTags="this.study.MainDicomTags"
                    :patientMainDicomTags="this.study.PatientMainDicomTags" :labels="this.study.Labels" @deletedStudy="onDeletedStudy"></StudyDetails>
            </td>
        </tr>
    </tbody>
</template>

<style scoped>
.study-row-collapsed {
    border-top-width: 1px;
    border-color: #ddd;
}

.study-row-expanded {
    background-color: var(--study-details-bg-color);
    font-weight: 700;

    border-top: 3px !important;
    border-style: solid !important;
    border-color: black !important;
}

.study-row-expanded>:first-child {
    border-bottom: 5px !important;
}

.study-row-show-labels {
    border-bottom: 0px !important;
}


.study-table>tbody>tr.study-row-expanded:hover {
    background-color: var(--study-details-bg-color);
    color: red;
}

.study-table>tbody>tr.study-details-expanded:hover {
    background-color: var(--study-details-bg-color);
    color: red;
}

.study-details-expanded {
    background-color: var(--study-details-bg-color);

    border-top: 0px !important;
    border-bottom: 3px !important;
    border-style: solid !important;
    border-color: black !important;
}

.label {
    margin-left: 2px;
    margin-left: 2px;
}

.label-row {
    border-top: 0px !important;
    border-bottom: 0px !important;
}

.td-viewer-icon {
    padding: 0; /* to maximize click space for the icon */
}

.td-pdf-icon {
    padding: 0; /* to maximize click space for the icon */
}
</style>