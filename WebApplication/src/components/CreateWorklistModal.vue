<script>
import { mapState } from "vuex"
import resourceHelpers from "../helpers/resource-helpers"
import dateHelpers from "../helpers/date-helpers"
import api from "../orthancApi"
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js"
import { translateDicomTag } from "../locales/i18n"


// these tags can not be removed
document.worklistsRequiredTags = [
    'PatientID', 'PatientName', 'ScheduledProcedureStepStartDate', 'Modality'
];


export default {
    props: ["currentId", "orthancStudyId", "modifyWorklistId", "wlFullTags", "reloadWindowAfterCreation"],
    data() {
        return {
            editableTags: [],                              // the tag values entered in the dialog
            samePatientStudiesCount: 0,
            foundPatientSameIdDifferentTags: false,
            samePatientIdTags: {},
            allValid: false,
            disablePatientNameComputation: true,
            formValidityCheckerHandler: null,
            errorMessage: null
        }
    },
    async mounted() {
        this.$refs['modal-main-div'].addEventListener('show.bs.modal', (e) => {
            // move the modal to body to avoid z-index issues: https://weblog.west-wind.com/posts/2016/sep/14/bootstrap-modal-dialog-showing-under-modal-background
            document.querySelector('body').appendChild(e.target);
            this.reset();
        });
    },
    methods: {
        async createEditableTag(tag, tagName, flattenedTagName, level, closeOpenSequence) {
            const isDate = dateHelpers.isDateTag(tagName);
            let defaultValue = ("DefaultValue" in tag ? tag["DefaultValue"] : "");
            if (isDate) {
                if (defaultValue == "$TODAY$") {
                    defaultValue = new Date();
                } else {
                    defaultValue = dateHelpers.fromDicomDate(defaultValue);
                }
            } else if (defaultValue == "$GENERATE_DICOM_UUID$") {
                defaultValue = await api.generateUid('study');
            }

            if (tagName == "PatientName" && tag["PatientNameType"] == "Computed") {
                let parts = tag["ComputedFrom"];

                return [{
                    "value": defaultValue,
                    "tagName": parts[0],
                    "flattenedTagName": flattenedTagName + "." + parts[0],
                    "level": level,
                    "isDate": false,
                    "isChoice": false,
                    "choices" : [],
                    "isEditable": true,
                    "isRequired": null,
                    "isComputationSource": true,
                    "isValid": false,
                    "placeholder": null,
                    "closeOpenSequence": null
                }, {
                    "value": defaultValue,
                    "tagName": parts[1],
                    "flattenedTagName": flattenedTagName + "." + parts[1],
                    "level": level,
                    "isDate": false,
                    "isChoice": false,
                    "choices" : [],
                    "isEditable": true,
                    "isRequired": null,
                    "isComputationSource": true,
                    "isValid": false,
                    "placeholder": null,
                    "closeOpenSequence": null
                }, {
                    "value": defaultValue,
                    "tagName": tagName,
                    "flattenedTagName": flattenedTagName,
                    "level": level,
                    "isDate": false,
                    "isChoice": false,
                    "choices" : [],
                    "isEditable": false,
                    "isRequired": ("Required" in tag ? tag["Required"] : false),
                    "isComputationSource": false,
                    "isValid": false,
                    "placeholder": null,
                    "closeOpenSequence": null,
                    "computedFrom": parts
                }];

            } else {
                return [{
                    "value": defaultValue,
                    "tagName": tagName,
                    "flattenedTagName": flattenedTagName,
                    "level": level,
                    "isDate": isDate,
                    "isChoice": "Choices" in tag,
                    "choices" : ("Choices" in tag ? tag["Choices"] : []),
                    "isEditable": ("Editable" in tag ? tag["Editable"] : true),
                    "isRequired": ("Required" in tag ? tag["Required"] : false),
                    "isComputationSource": false,
                    "isValid": false,
                    "placeholder": tag["Placeholder"],
                    "closeOpenSequence": closeOpenSequence
                }];                
            }
        },
        async flattenAndUpdateKeywords(tags, prefix, level) {
            let outTags = [];
            for (const [k, v] of Object.entries(tags)) {
                const flattenedTagName = (prefix ? prefix + "." + k : k);

                const defaultValue = ("DefaultValue" in v ? v["DefaultValue"] : "");
                if (Object.prototype.toString.call(defaultValue) === '[object Object]') {
                    outTags = outTags.concat(await this.flattenAndUpdateKeywords(defaultValue, flattenedTagName, level + 1));
                } else if (Object.prototype.toString.call(defaultValue) === '[object Array]') {
                    outTags = outTags.concat(await this.createEditableTag({}, k, flattenedTagName, level, "open"));
                    for (const e of defaultValue) {
                        outTags = outTags.concat(await this.flattenAndUpdateKeywords(e, flattenedTagName, level + 1));
                    }
                    // outTags.push(await this.createEditableTag({}, k, flattenedTagName, level, "close"));
                } else {
                    outTags = outTags.concat(await this.createEditableTag(v, k, flattenedTagName, level, null));
                }
            }
            return outTags;
        },
        unflattenTags(editableTags) {
            let outTags = {};
            
            for (let t of editableTags) {
                if (t["isComputationSource"]) { // don't include these fields in the payload
                    continue;
                }

                let value;
                if (t["isDate"]) {
                    value = dateHelpers.dicomDateFromDatePicker(t["value"]);
                } else {
                    value = t["value"];
                }

                if (t["level"] == 0) {
                    if (t["closeOpenSequence"] == "open") {
                        outTags[t["tagName"]] = []
                        outTags[t["tagName"]].push({});
                    } else {
                        outTags[t["tagName"]] = value;
                    }
                } else if (t["level"] == 1) {
                    const s = t["flattenedTagName"].split(".");
                    outTags[s[0]][outTags[s[0]].length-1][s[1]] = value;
                }
            }
            return outTags;
        },
        mergeTagsIntoEditableTags(wlFullTags) {
            for (let t of this.editableTags) {
                for (const [k, v] of Object.entries(wlFullTags)) {
                    if (t["level"] == 0) {
                        if (t["tagName"] == v["Name"]) {
                            if (t["isDate"]) {
                                t["value"] = dateHelpers.fromDicomDate(v["Value"]);
                            } else {
                                t["value"] = v["Value"];
                            }
                        }
                    } else if (t["level"] == 1 && v["Name"] == "ScheduledProcedureStepSequence") {
                        for (const s of v["Value"]) {
                            for (const [k1, v1] of Object.entries(s)) {
                                if (t["tagName"] == v1["Name"]) {
                                    if (t["isDate"]) {
                                        t["value"] = dateHelpers.fromDicomDate(v1["Value"]);
                                    } else {
                                        t["value"] = v1["Value"];
                                    }
                                }
                            }
                        }
                    }
                }
            }
        },
        async reset() {
            this.editableTags = await (this.flattenAndUpdateKeywords(this.uiOptions.NewWorklistDefaultTags, null, 0));

            if (this.modifyWorklistId && this.wlFullTags) {
                this.mergeTagsIntoEditableTags(this.wlFullTags);

            } else if (this.orthancStudyId) {
                let study = await api.getStudy(this.orthancStudyId);
                for (let t of this.editableTags) {
                    for (const [k, v] of Object.entries(study.PatientMainDicomTags)) {
                        if (t["tagName"] == k) {
                            if (t["isDate"]) {
                                t["value"] = dateHelpers.fromDicomDate(v);
                            } else {
                                t["value"] = v;
                            }
                        }
                    }
                }
            }

            this.updateValidity();
        },
        async updateValidity() {
            // console.log("updating validity");
            let _allValid = true;
            let wlPatientTags = {}

            for (let t of this.editableTags) {
                if (t["isRequired"] && !t["value"]) {
                    t["isValid"] = false;
                    _allValid = false;
                } else {
                    t["isValid"] = true;
                }
                if (resourceHelpers.isPatientTag(t["tagName"])) {
                    wlPatientTags[t["tagName"]] = t["value"];
                }
            }
            this.allValid = _allValid;
            this.foundPatientSameIdDifferentTags = false;

            if (wlPatientTags["PatientID"]) {
                const targetPatient = await api.findPatient(wlPatientTags["PatientID"]);
                if (targetPatient) {
                    console.log("found patient", targetPatient);

                    this.samePatientIdTags = targetPatient["MainDicomTags"];
                    let _allIdentical = true;
                    for (const [k, v] of Object.entries(this.samePatientIdTags)) {
                        if (k == "PatientBirthDate") {
                            if (wlPatientTags[k] && v && (wlPatientTags[k].toISOString() != dateHelpers.fromDicomDate(v).toISOString())) {
                                _allIdentical = false;
                            }
                        } 
                        else if (wlPatientTags[k] != v) {
                            _allIdentical = false;
                        }
                    }

                    this.foundPatientSameIdDifferentTags = !_allIdentical;
                }
            }
        },
        copySamePatientTags() {
            this.disablePatientNameComputation = true; // temporarily while we copy the value and the next watch() is executed
            for (let t of this.editableTags) {
                if (resourceHelpers.isPatientTag(t["tagName"]) && t["tagName"] in this.samePatientIdTags) {
                    if (t["isDate"]) {
                        t["value"] = dateHelpers.fromDicomDate(this.samePatientIdTags[t["tagName"]]);
                    } else {
                        t["value"] = this.samePatientIdTags[t["tagName"]];
                    }
                } else if (t["isComputationSource"]) {
                    t["value"] = "";
                }
            }
        },
        async apply(event) {
            try {
                let apiTags = this.unflattenTags(this.editableTags);
                let msg;
                if (this.modifyWorklistId) {
                    await api.updateWorklist(this.modifyWorklistId, apiTags);
                    msg = 'worklists.modified';
                } else {
                    await api.createWorklist(apiTags);
                    msg = 'worklists.created';
                }
                if (this.reloadWindowAfterCreation) {
                    window.location.reload();
                } else {
                    var modal = bootstrap.Modal.getInstance(this.$refs['modal-main-div']);
                    modal.hide();
                    this.messageBus.emit("show-toast", this.$t(msg));
                }
            } catch (err) {
                if (err.response && err.response.data && err.response.data.Message) {
                    this.errorMessage = "Error creating/updating a worklist: " + err.response.data.Message;
                } else {
                    this.errorMessage = "Error creating/updating a worklist"
                }
                console.error("Error creating/updating a worklist", err);
            }
        },
        translate(tagName) {
            return translateDicomTag(this.$i18n.t, this.$i18n.te, tagName);
        },
        handleInput(isComputationSource) {
            if (!isComputationSource) {
                return;
            }

            // update the PatientName
            for (let t of this.editableTags) {
                if (t["computedFrom"]) {
                    let values = [];
                    for (const part of t["computedFrom"]) {
                        let partValue = this.editableTags.filter(s => s["tagName"] == part)[0]["value"];
                        if (partValue && partValue != "") {
                            values.push(partValue);
                        }
                    }
                    if (values.length > 0) {
                        t["value"] = values.join("^");
                    } else {
                        t["value"] = "";
                    }
                }
            }
        }
        
    },
    watch: {
        editableTags: {
            handler(newValue, oldValue) {
                // reactivate PatientNameComputation as soon as we modify the source of the computation
                console.log(newValue, oldValue);
                // if (this.disablePatientNameComputation) {
                //     for (let t of this.editableTags) {
                //         if (t["computedFrom"]) {
                //             let values = [];
                //             for (const part of t["computedFrom"]) {
                //                 let partValue = this.editableTags.filter(s => s["tagName"] == part)[0]["value"];
                //                 if (partValue && partValue != "") {
                //                     values.push(partValue);
                //                 }
                //             }
                //             if (values.length > 0) {
                //                 t["value"] = values.join("^");
                //             } else {
                //                 t["value"] = "";
                //             }
                //         }
                //     }
                // }

                // if (!this.disablePatientNameComputation) {
                //     for (let t of this.editableTags) {
                //         if (t["computedFrom"]) {
                //             let values = [];
                //             for (const part of t["computedFrom"]) {
                //                 let partValue = this.editableTags.filter(s => s["tagName"] == part)[0]["value"];
                //                 if (partValue && partValue != "") {
                //                     values.push(partValue);
                //                 }
                //             }
                //             if (values.length > 0) {
                //                 t["value"] = values.join("^");
                //             } else {
                //                 t["value"] = "";
                //             }
                //         }
                //     }
                // }

                // only trigger form validation when we stop typing for 300ms
                if (this.formValidityCheckerHandler) {
                    clearTimeout(this.formValidityCheckerHandler);
                }
                this.formValidityCheckerHandler = setTimeout(() => { this.updateValidity() }, 300);
            },
            deep: true
        },

    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions
        }),
        isDarkMode() {
            // hack to switch the theme: get the value from our custom css
            let bootstrapTheme = document.documentElement.getAttribute("data-bs-theme"); // for production
            bootstrapTheme = getComputedStyle(document.documentElement).getPropertyValue('--bootstrap-theme');  // for dev
            console.log("DatePicker color mode is ", bootstrapTheme);
            return bootstrapTheme == "dark";
        },
        datePickerFormat() {
            return this.uiOptions.DateFormat;
        }        
    },
    components: {  }

};
</script>

<template>
    <div class="modal fade" id="create-worklist-modal" tabindex="-1" aria-labelledby="modalLabel" ref="modal-main-div">
        <!-- aria-hidden="true" -->
        <div class="modal-dialog modal-lg modal-dialog-scrollable">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title" id="modalLabel">{{ $t("worklists.create_title") }} </h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>

                <!-- ------------------------------------------ step 'tags' --------------------------------------------------->
                <div class="modal-body">
                    <div class="container" style="min-height: 70vh"> <!-- min height for the date picker-->
                        <div v-for="editableTag of editableTags" :key="editableTag.flattenedTagName" class="row py-1">
                            <div v-if="editableTag.closeOpenSequence == null" class="col-md-5">
                                {{ translate(editableTag.tagName) }}
                            </div>
                            <div v-if="editableTag.isDate" class="col-md-6">
                                <Datepicker v-model="editableTag.value" :range="false"
                                    :enable-time-picker="false" :format="datePickerFormat"
                                    :preview-format="datePickerFormat" hide-input-icon text-input arrow-navigation :disabled="!editableTag.isEditable" :highlight="{ weekdays: [6, 0]}" :dark="isDarkMode">
                                </Datepicker>
                            </div>
                            <div v-else-if="editableTag.isChoice" class="col-md-6 text-end">
                                <select v-model="editableTag.value" class="form-select-sm" :disabled="!editableTag.isEditable">
                                    <option v-for="choice of editableTag.choices" :key="choice" :value="choice">{{ choice }}</option>
                                </select>
                            </div>
                            <div v-else-if="editableTag.closeOpenSequence == null" class="col-md-6">
                                <input v-if="true" type="text" class="form-control" v-model="editableTag.value" :disabled="!editableTag.isEditable" :placeholder="editableTag.placeholder" @input="handleInput(editableTag.isComputationSource)" />
                            </div>
                            <div v-if="!editableTag.isComputationSource && editableTag.closeOpenSequence == null && editableTag.isValid" class="col-md-1">
                                <i style="color: green" class="bi-check"></i>
                            </div>
                            <div v-else-if="!editableTag.isComputationSource && editableTag.closeOpenSequence == null" class="col-md-1">
                                <i style="color: red" class="bi-x-lg"></i>
                            </div>
                        </div>
                        <div v-if="foundPatientSameIdDifferentTags" class="mt-2">
                             <div class="alert alert-warning" role="alert">
                                <p>{{ $t('worklists.same_patient_id_found_different_tags') }}</p>
                                <p><span v-for="(v, k) in samePatientIdTags"><strong>{{ k }}:</strong> {{ v }}<br/></span></p>
                                <p><button type="button" class="btn btn-primary" @click="copySamePatientTags()">{{ $t("worklists.copy_tags_from_patient") }}</button></p>
                             </div>
                        </div>
                        <div v-if="errorMessage" class="mt-2">
                             <div class="alert alert-danger" role="alert">
                                <p>{{ errorMessage }}</p>
                             </div>
                        </div>
                    </div>
                </div>

                
                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">{{ $t("cancel") }}</button>
                    <button type="button" class="btn btn-primary" @click="apply($event)" :disabled="!allValid">
                        <span v-if="this.modifyWorklistId">{{$t("worklists.modify_button")}}</span>
                        <span v-else>{{$t("worklists.create_button")}}</span>
                    </button>
                </div>
            </div>
        </div>
    </div>
</template>

<style>
/* The switch - the box around the slider */
.switch {
    position: relative;
    display: inline-block;
    width: 60px;
    height: 34px;
}

/* Hide default HTML checkbox */
.switch input {
    opacity: 0;
    width: 0;
    height: 0;
}

/* The slider */
.slider {
    position: absolute;
    cursor: pointer;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: #ccc;
    -webkit-transition: .4s;
    transition: .4s;
}

.slider:before {
    position: absolute;
    content: "";
    height: 26px;
    width: 26px;
    left: 4px;
    bottom: 4px;
    background-color: white;
    -webkit-transition: .4s;
    transition: .4s;
}

input:checked+.slider {
    background-color: #2196F3;
}

input:focus+.slider {
    box-shadow: 0 0 1px #2196F3;
}

input:checked+.slider:before {
    -webkit-transform: translateX(26px);
    -ms-transform: translateX(26px);
    transform: translateX(26px);
}

/* Rounded sliders */
.slider.round {
    border-radius: 34px;
}

.slider.round:before {
    border-radius: 50%;
}

.disabled-text {
    color: #888;
}

.btn-small {
    line-height: var(--bs-body-line-height);
    border: 0;
    background-color: transparent;
    border-radius: 0.2rem;
}

.striked-through {
    text-decoration: line-through;
}
</style>