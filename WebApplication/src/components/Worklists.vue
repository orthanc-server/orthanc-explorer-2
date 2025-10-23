<script>

import { mapState } from "vuex"
import api from "../orthancApi"
import { translateDicomTag } from "../locales/i18n"
import dateHelpers from "../helpers/date-helpers"
import resourceHelpers from "../helpers/resource-helpers";
import Modal from "./Modal.vue"
import TagsTree from "./TagsTree.vue";

document._worklistsColumns = {
    "StudyDate": {
        "width": "7%",
        "isOrderable": true
    },
    "AccessionNumber": {
        "width": "11%",
        "isOrderable": true
    },
    "PatientID": {
        "width": "11%",
        "isOrderable": true
    },
    "PatientName": {
        "width": "15%",
        "isOrderable": true
    },
    "PatientBirthDate": {
        "width": "7%",
        "isOrderable": true
    },
    "startDate": {
        "width": "25%",
        "isOrderable": true
    },
    "description": {
        "width": "25%",
        "isOrderable": true
    },
    "undefined": {
        "width": "10%",
        "isOrderable": false
    },
    "PatientSex": {
        "width": "7%",
        "isOrderable": true
    },
    "OtherPatientIDs": {
        "width": "10%",
        "isOrderable": true
    },
    "InstitutionName": {
        "width": "12%",
        "isOrderable": true
    },
    "ReferringPhysician": {
        "width": "10%",
        "isOrderable": true
    },
    "RequestingPhysician": {
        "width": "10%",
        "isOrderable": true
    }
};



export default {
    props: [],
    emits: [],
    data() {
        return {
            worklists: [],
            columns: document._studyColumns,            
            currentOrderByTag: null,
            currentOrderDirection: 'ASC',
            allSelected: false,
            isPartialSelected: false
        };
    },
    async mounted() {
        let worklists_ = await api.getWorklists();
        for (const wl of worklists_) {
            let shortTags = {};
            for (const [k, v] of Object.entries(wl.Tags)) {
                shortTags[v["Name"]] = v["Value"];
                if (v["Name"] == "ScheduledProcedureStepSequence") {
                    for (const s of v["Value"]) {
                        for (const [sk, sv] of Object.entries(s)) {
                            if (sv["Name"] == "ScheduledProcedureStepStartDate" && !("startDate" in shortTags)) {
                                shortTags["startDate"] = sv["Value"];
                            } else if (sv["Name"] == "ScheduledProcedureStepDescription" && !("description" in shortTags)) {
                                shortTags["description"] = sv["Value"];
                            } else if (sv["Name"] == "Modality") {
                                if (!("modalities" in shortTags)) {
                                    shortTags["modalities"] = sv["Value"];
                                } else {
                                    shortTags["modalities"] = shortTags["modalities"] + "," + sv["Value"];
                                }
                            }
                        }
                    }
                }
            }
            this.worklists.push({
                "ID": wl.ID,
                "FullTags": wl.Tags,
                "ShortTags": shortTags,
                "Expanded": false,
                "Selected": false
            });
        }
    },
    methods: {
        getTag(wl, columnTag) {
            if (columnTag in wl.ShortTags && wl.ShortTags[columnTag]) {
                return wl.ShortTags[columnTag];
            } else {
                return "";
            }
        },
        columnTitle(tagName) {
            return translateDicomTag(this.$i18n.t, this.$i18n.te, tagName);
        },
        columnTooltip(tagName) {
            return this.columnTitle(tagName);
        },
        columnWidth(tagName) {
            if (tagName in this.columns) {
                return this.columns[tagName].width;
            } else {
                return this.columns["undefined"].width;
            }
        },
        isOrderable(tagName) {
            if (tagName in document._worklistsColumns) { 
                return document._worklistsColumns[tagName].isOrderable;
            } else {
                return false;
            }
        },
        isOrderTagUp(tagName) {
            return tagName == this.currentOrderByTag && this.currentOrderDirection == 'DESC';
        },
        isOrderTagDown(tagName) {
            return tagName == this.currentOrderByTag && this.currentOrderDirection == 'ASC';
        },
        toggleOrder(ev, tagName) {
            ev.preventDefault();
            ev.stopPropagation();

            if (this.currentOrderByTag == tagName) { // this tag is already used for ordering -> change order direction
                this.currentOrderDirection = (this.currentOrderDirection == 'ASC' ? 'DESC' : 'ASC');
            } else { // this tag is not used for ordering
                this.currentOrderByTag = tagName;
                this.currentOrderDirection = 'ASC';
            }
            this.worklists.sort((a, b) => (this.currentOrderDirection == 'ASC' ?
                                           this.getTag(a, this.currentOrderByTag).localeCompare(this.getTag(b, this.currentOrderByTag)) :
                                           -this.getTag(a, this.currentOrderByTag).localeCompare(this.getTag(b, this.currentOrderByTag))));

        },
        clickSelectAll() {
            if (this.allSelected) {
                this.allSelected = false;
                for (const kw in this.worklists) {
                    this.worklists[kw].Selected = false;
                }
                this.isPartialSelected = false;
            } else {
                this.allSelected = true;
                for (const kw in this.worklists) {
                    this.worklists[kw].Selected = true;
                }
                this.isPartialSelected = false;
            }
        },
        updateAllSelected() {
            let allSelected = true;
            let noneSelected = true;
            for (const kw in this.worklists) {
                allSelected &= this.worklists[kw].Selected;
                noneSelected &= !this.worklists[kw].Selected;
            }
            this.allSelected = allSelected == 1;
            this.isPartialSelected = !(allSelected == 1) && !(noneSelected == 1);
        },
        formattedTag(wl, columnTag) {
            if (["PatientName"].indexOf(columnTag) != -1) {
                return resourceHelpers.formatPatientName(this.getTag(wl, columnTag));
            } else if (["startDate", "PatientBirthDate"].indexOf(columnTag) != -1) {
                return dateHelpers.formatDateForDisplay(this.getTag(wl, columnTag), this.uiOptions.DateFormat);
            }
            return this.getTag(wl, columnTag);
        },
        async deleteSelectedWorklists() {
            for (const w of this.worklists) {
                if (w.Selected) {
                    await api.deleteWorklist(w.ID);
                }
            }
            window.location.reload();
        },
        async deleteWorklist(event, wl) {
            await api.deleteWorklist(wl.ID);
            window.location.reload();
        }
    },
    watch: {
        worklists: {
            handler(newValue, oldValue) {
                this.updateAllSelected();
            },
            deep: true
        },
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
        }),
        selectedWorklistsCount() {
            let count = 0;
            for (const w of this.worklists) {
                if (w.Selected) {
                    count += 1;
                }
            }
            if (count > 0) {
                return count;
            } else {
                return "";
            }
        },
        colSpanActions() {
            if (this.uiOptions && this.uiOptions.WorklistsColumns) {
                return this.uiOptions.WorklistsColumns.length;
            } else {
                return 5;
            }
        },
        colSpanDetails() {
            return 1 + this.colSpanActions;
        }

    },
    components: { Modal, TagsTree }
}
</script>
<template>
    <div>
        <table class="table table-sm study-table table-borderless">
            <thead class="sticky-top">
                <tr class="worklists-column-titles">
                    <th width="2%" max-width="40px" scope="col"></th>
                    <th v-for="columnTag in uiOptions.WorklistsColumns" :key="columnTag" data-bs-toggle="tooltip"
                        v-bind:title="columnTooltip(columnTag)" v-bind:width="columnWidth(columnTag)"
                        class="study-table-title">
                        <div class="title-container">
                            <div v-if="isOrderable(columnTag)" class="title-text is-orderable" @click="toggleOrder($event, columnTag)">{{ columnTitle(columnTag) }}</div>
                            <div v-if="!isOrderable(columnTag)" class="title-text">{{ columnTitle(columnTag) }}</div>
                            <div v-if="isOrderTagUp(columnTag)" class="title-arrow"><i class="bi bi-arrow-up"></i></div>
                            <div v-if="isOrderTagDown(columnTag)" class="title-arrow"><i class="bi bi-arrow-down"></i></div>
                        </div>
                    </th>
                </tr>
                <tr class="worklists-table-actions">
                    <th width="2%" scope="col">
                        <div class="form-check" style="margin-left: 0.5rem">
                            <input class="form-check-input" type="checkbox" v-model="allSelected"
                                :indeterminate="isPartialSelected" @click="clickSelectAll"><span style="font-weight: 400; font-size: small;">{{ selectedWorklistsCount }}</span>
                        </div>
                    </th>
                    <th width="98%" :colspan="colSpanActions" scope="col">
                        <div class="container px-0">
                            <div class="btn-group">
                                <button class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="modal"
                                    v-bind:data-bs-target="'#delete-modal-worklists'" :disabled="selectedWorklistsCount == 0">
                                    <i class="bi bi-trash" data-bs-toggle="tooltip" :title="$t('delete')"></i>
                                </button>
                                <Modal id="delete-modal-worklists"
                                    :headerText="$t('worklists.delete_bulk_worklists_title')" :okText="$t('delete')"
                                    :cancelText="$t('cancel')" :bodyText="$t('worklists.delete_bulk_worklists_body', {'count': selectedWorklistsCount})" @ok="deleteSelectedWorklists($event)">
                                </Modal>
                            </div>
                        </div>
                    </th>
                </tr>
            </thead>
            <tbody v-for="wl in worklists" :key="wl.ID">
                <tr :class="{ 'worklist-row-collapsed': !wl.Expanded, 'worklist-row-expanded': wl.Expanded }" >
                    <td>
                        <div class="form-check">
                            <input class="form-check-input" type="checkbox" v-model="wl.Selected">
                        </div>
                    </td>
                    <td v-for="columnTag in uiOptions.WorklistsColumns" :key="columnTag" class="cut-text" data-bs-toggle="collapse"
                v-bind:data-bs-target="'#worklist-details-' + wl.ID"
                    :title="formattedTag(wl, columnTag)">
                        {{ formattedTag(wl, columnTag) }}
                    </td>
                </tr>
                <tr class="collapse worklists-details-expanded"
                    v-bind:id="'worklist-details-' + wl.ID">
                    <td :colspan="colSpanDetails">
                        <div class="container mt-4 px-5">
                            <div class="row">
                                <div class="col-8">
                                </div>
                                <div class="col-4">
                                    <button class="btn btn-sm btn-secondary m-1" type="button" data-bs-toggle="modal"
                                        v-bind:data-bs-target="'#delete-modal-worklist-' + wl.ID">
                                        <i class="bi bi-trash" data-bs-toggle="tooltip" :title="$t('delete')"></i>
                                    </button>
                                    <Modal :id="'delete-modal-worklist-' + wl.ID"
                                        :headerText="$t('worklists.delete_single_worklist_title')" :okText="$t('delete')"
                                        :cancelText="$t('cancel')" :bodyText="$t('worklists.delete_single_worklist_body')" @ok="deleteWorklist($event, wl)">
                                    </Modal>
                                </div>
                            </div>
                            <div class="row">
                                <TagsTree 
                                    :tags="wl.FullTags">
                                </TagsTree>
                            </div>
                        </div>
                    </td>
                </tr>
            </tbody>
        </table>
    </div>
</template>
<style scoped>
.worklists-column-titles {
  background-color: var(--study-table-header-bg-color) !important;
  font-size: smaller;
}

.worklists-row-collapsed {
    border-top-width: 1px;
    border-color: #ddd;
}

.worklists-row-expanded {
    background-color: var(--study-details-bg-color);
    font-weight: 700;

    border-top: 3px !important;
    border-style: solid !important;
    border-color: black !important;
}

.worklists-row-expanded>:first-child {
    border-bottom: 5px !important;
}

.worklists-row-show-labels {
    border-bottom: 0px !important;
}


.worklists-table>tbody>tr.worklists-row-expanded:hover {
    background-color: var(--study-details-bg-color);
    color: red;
}

.worklists-table>tbody>tr.worklists-details-expanded:hover {
    background-color: var(--study-details-bg-color);
    color: red;
}

.worklists-details-expanded {
    margin-top: 2rem;;
    background-color: var(--study-details-bg-color);
    font-size: 0.8rem;

    border-top: 0px !important;
    border-bottom: 3px !important;
    border-style: solid !important;
    border-color: black !important;
}

.worklists-table-actions > th {
  background-color: var(--study-table-actions-bg-color) !important;
  vertical-align: middle;
}

.worklists-table-actions > th > div {
  background-color: var(--study-table-actions-bg-color) !important;
  text-align: left;
}

</style>