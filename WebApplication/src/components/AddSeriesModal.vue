<script>
import { uppie } from "uppie"
import { mapState } from "vuex"
import resourceHelpers from "../helpers/resource-helpers"
import dateHelpers from "../helpers/date-helpers"
import api from "../orthancApi"
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js"



export default {
    props: ["orthancStudyId", "studyMainDicomTags", "patientMainDicomTags"],
    data() {
        return {
            seriesTags: {},                                      // the series tag values entered in the dialog
            seriesDateTags: {},                                  // same as tags but only for dates (for the DatePicker)
            errorMessageId: null,
            warningMessageId: null,
            uploadedFileType: null,
            uploadedFileDate: null,
            uploadedFileBase64Content: null,
            uploadedFileHumanSize: null,
            step: 'prepare', // allowed values: 'prepare', 'error'
        }
    },
    async mounted() {
        this.$refs['modal-main-div'].addEventListener('show.bs.modal', (e) => {
            // move the modal to body to avoid z-index issues: https://weblog.west-wind.com/posts/2016/sep/14/bootstrap-modal-dialog-showing-under-modal-background
            document.querySelector('body').appendChild(e.target);
            this.reset();
        });
        uppie(document.querySelector("#addSeriesFileUpload-" + this.orthancStudyId), this.uppieUploadHandler);
    },
    methods: {
        async reset() {
            this.step = 'prepare';
            this.uploadedFileType = null;
            this.uploadedFileDate = null;
            this.uploadedFileHumanSize = null;
            this.uploadedFileName = null;
            this.uploadedFileBase64Content = null;
            this.warningMessageId = null;
            this.errorMessageId = null;
            this.seriesTags = {};
            document.getElementById("addSeriesFileUpload-" + this.orthancStudyId).value = null;
        },
        setError(errorMessageId) {
            this.step = 'error';
            this.errorMessageId = errorMessageId;
        },
        async addSeries() {
            try {
                let tags = {};

                for (const [k,] of Object.entries(this.seriesTags)) {
                    if (dateHelpers.isDateTag(k)) {
                        tags[k] = dateHelpers.dicomDateFromDatePicker(this.seriesDateTags[k]);
                    } else {
                        tags[k] = this.seriesTags[k];
                    }
                }


                // if no AcquisitionDate and ContentDate, copy it from SeriesDate ...
                if (!("AcquisitionDate" in tags)) {
                    tags["AcquisitionDate"] = tags["SeriesDate"]
                }
                if (!("ContentDate" in tags)) {
                    tags["ContentDate"] = tags["SeriesDate"]
                }

                await api.createDicom(this.orthancStudyId, this.uploadedFileBase64Content, tags);
                let closeButton = document.getElementById('add-series-close-'+ this.orthancStudyId);
                closeButton.click();
                this.messageBus.emit('added-series-to-study-' + this.orthancStudyId);
            } catch (err) {
                this.setError('add_series.error_add_series_unexpected_error_html');
            }
        },
        isDateTag(tag) {
            return dateHelpers.isDateTag(tag);
        },
        onDrop(ev) {
            ev.preventDefault();
            this.analyzeFile(ev.dataTransfer.items[0].getAsFile());
        },
        onDragOver(event) {
            event.preventDefault();
        },
        getHumanSize(sizeInBytes) {
            if (sizeInBytes > 1024*1024) {
                return (Math.round(sizeInBytes/(1024*1024)*100) / 100) + " MB"; 
            } else {
                return (Math.round(sizeInBytes/1024*100) / 100) + " kB"; 
            }
        },
        async uppieUploadHandler(event, formData, files) {
            event.preventDefault();
            this.analyzeFile(event.target.files[0]);
        },
        async analyzeFile(file) {
            console.log("analyzeFile", file);
            this.uploadedFileDate = file.lastModifiedDate;
            this.uploadedFileHumanSize = this.getHumanSize(file.size);
            this.uploadedFileName = file.name;
            this.warningMessageId = null;

            if (file.type == 'application/pdf') {
                this.uploadedFileType = "pdf";
            } else if (file.type.startsWith('image/')) {
                this.uploadedFileType = "image";
            } else if (file.type == "" && file.name.endsWith ) {
                this.uploadedFileType = "stl"
            } else {
                this.uploadedFileType = null;
                this.warningMessageId = "add_series.unrecognized_file_format"
                return;
            }

            for (const [k, v] of Object.entries(this.uiOptions.AddSeriesDefaultTags[this.uploadedFileType])) {
                this.seriesTags[k] = v;
                if (dateHelpers.isDateTag(k)) {
                    if (v == '$TODAY$') {
                        this.seriesDateTags[k] = new Date();
                    } else if (v == '$STUDY_DATE$') {
                        this.seriesDateTags[k] = dateHelpers.fromDicomDate(this.studyMainDicomTags["StudyDate"]);
                    } else if (v == '$FILE_DATE$') {
                        this.seriesDateTags[k] = this.uploadedFileDate;
                    } else {
                        this.seriesDateTags[k] = dateHelpers.fromDicomDate(v);
                    }
                }
            }

            let reader = new FileReader();
            let that = this;
            reader.onload = function (event) {
                that.uploadedFileBase64Content = event.target.result;
            }
            reader.readAsDataURL(file);
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions
        }),
        resourceTitle() {
            return resourceHelpers.getResourceTitle('study', this.patientMainDicomTags, this.studyMainDicomTags, null, null);
        },
        addSeriesButtonEnabled() {
            return this.uploadedFileType != null;
        },
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
    components: {}

};
</script>

<template>
    <div class="modal fade" :id="this.orthancStudyId" tabindex="-1" aria-labelledby="modalLabel" ref="modal-main-div">
        <!-- aria-hidden="true" -->
        <div class="modal-dialog modal-lg modal-dialog-scrollable">
            <div class="modal-content">
                <div class="modal-header">
                    <h5 class="modal-title" id="modalLabel">{{ $t("add_series.modal_title") + " " + resourceTitle }}
                    </h5>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close" :id="'add-series-close-'+ orthancStudyId"></button>
                </div>
                <!-- ------------------------------------------ step 'prepare' --------------------------------------------------->
                <div v-if="step == 'prepare'" class="modal-body">
                    <div class="upload-drop-zone" @drop="this.onDrop" @dragover="this.onDragOver">
                        <div class="mb-3">{{ $t('add_series.drop_file') }}</div>
                        <div class="mb-3">
                            <label class="btn btn-primary btn-file">
                                {{ $t('add_series.select_file') }} <input type="file" style="display: none;"
                                    :id="'addSeriesFileUpload-' + this.orthancStudyId" required>
                            </label>
                        </div>
                    </div>
                    <div v-if="warningMessageId != null" class="modal-body">
                        <div class="container">
                            <div class="alert alert-warning" role="alert" v-html="$t(warningMessageId)">
                            </div>
                        </div>
                    </div>
                    <div v-if="uploadedFileType != null" class="modal-body">
                        <div class="container">
                            <div class="alert alert-info" role="alert" v-html="$t('add_series.uploaded_file', {'size': uploadedFileHumanSize, 'type': uploadedFileType, 'name': uploadedFileName})">
                            </div>
                        </div>
                    </div>
                    <div v-if="uploadedFileType != null" class="container" style="min-height: 40vh">
                        <!-- min height for the date picker-->
                        <div v-for="(item, key) in seriesTags" :key="key" class="row">
                            <!----  label  ---->
                            <div class="col-md-5">
                                {{ key }}
                            </div>

                            <!----  edit text  ---->
                            <div v-if="!isDateTag(key)" class="col-md-6">
                                <input v-if="true" type="text" class="form-control" v-model="seriesTags[key]" />
                            </div>

                            <div v-if="isDateTag(key)" class="col-md-6">
                                <Datepicker v-model="seriesDateTags[key]" :range="false" :enable-time-picker="false"
                                    :format="datePickerFormat" hide-input-icon :preview-format="datePickerFormat" text-input
                                    arrow-navigation :highlight-week-days="[0, 6]" :dark="isDarkMode">
                                </Datepicker>
                            </div>
                        </div>
                    </div>
                </div>


                <div v-if="step == 'prepare'" class="modal-footer">
                    <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">{{ $t("cancel") }}</button>
                    <button type="button" class="btn btn-primary" :disabled="!addSeriesButtonEnabled"
                        @click="addSeries()">{{ $t('add_series.button_title') }}</button>
                </div>

                <!-- ------------------------------------------ step 'error' --------------------------------------------------->
                <div v-if="step == 'error'" class="modal-body">
                    <div class="container">
                        <div class="alert alert-danger" role="alert" v-html="$t(errorMessageId)">
                        </div>
                    </div>
                </div>
                <div v-if="step == 'error'" class="modal-footer">
                    <button type="button" class="btn btn-secondary" @click="back()">{{
        $t("modify.back_button_title")
                        }}</button>
                    <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">{{ $t("cancel") }}</button>
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

.upload-drop-zone {
    border-color: white;
    border-style: dashed;
    border-width: 4px;
    padding: 1rem;
    margin-top: 10px;
    margin-bottom: 1rem;
}
</style>