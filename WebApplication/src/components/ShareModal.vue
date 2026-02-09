<script>
import { mapState } from "vuex"
import CopyToClipboardButton from "./CopyToClipboardButton.vue";
import resourceHelpers from "../helpers/resource-helpers"
import clipboardHelpers from "../helpers/clipboard-helpers"
import api from "../orthancApi"
import axios from "axios";
import TextEditor from "./TextEditor.vue";



export default {
    props: ["id", "orthancId", "studyMainDicomTags", "patientMainDicomTags"],
    data() {
        return {
            shareLink: "",
            emailContent: "",
            emailTitle: "",
            emailDestination: ""
        }
    },
    async mounted() {
        this.reset();

        this.$refs['modal-main-div'].addEventListener('show.bs.modal', (e) => {
            // move the modal to body to avoid z-index issues: https://weblog.west-wind.com/posts/2016/sep/14/bootstrap-modal-dialog-showing-under-modal-background
            document.querySelector('body').appendChild(e.target);
            this.onShown();
        });
    },
    watch: {
    },
    methods: {
        async onShown() {
            this.reset();
            await this.share();

            if (this.enableShareByEmail) {
                let templateEmail = (await axios.get("/emails/templates/" + this.uiOptions.ShareEmailContentTemplate)).data;

                templateEmail = templateEmail.replace('{ShareUrl}', this.shareLink);
                templateEmail = await resourceHelpers.replaceResourceTagsInStringPlainText(templateEmail, this.patientMainDicomTags, this.studyMainDicomTags, {}, {}, this.orthancId, null, [], []);
                this.emailContent = templateEmail;
            }
        },
        async reset() {
            this.shareLink = "";
            this.emailDestination = "";
            this.emailTitle = this.uiOptions.ShareEmailTitle;
        },
        async share() {
            let resourcesIds = [this.orthancId];
            if (this.isBulkSelection) {
                resourcesIds = this.selectedStudiesIds;
            }
            let token = await api.createToken({
                tokenType: this.tokens.ShareType,  // defined in configuration file
                resourcesIds: resourcesIds,
                level: 'study',
                validityDuration: this.uiOptions.ShareDuration * 24 * 3600
            })
            this.shareLink = token["Url"];
        },
        copyAndClose() {
            clipboardHelpers.copyToClipboard(this.shareLink);
        },
        sendEmail() {
            // console.log("Sending email to ", this.emailContent);
            api.sendEmail(this.emailDestination, this.emailTitle, this.emailContent, this.uiOptions.ShareEmailLayoutTemplate);
        }
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            tokens: state => state.configuration.tokens,
            selectedStudiesIds: state => state.studies.selectedStudiesIds,
        }),
        isBulkSelection() {
            return !(this.patientMainDicomTags && this.studyMainDicomTags && Object.keys(this.patientMainDicomTags).length > 0 && Object.keys(this.studyMainDicomTags).length > 0);
        },
        resourceTitle() {
            if (!this.isBulkSelection) {
                return resourceHelpers.getResourceTitle("study", this.patientMainDicomTags, this.studyMainDicomTags, null, null);
            } else {
                return "";
            }
        },
        studiesCount() {
            return this.selectedStudiesIds.length;
        },
        enableShareByEmail() {
            return this.uiOptions.EnableSharesByEmail;
        },
        insertableTags() {
            if (this.isBulkSelection) {
                return {};
            }
            return {
                'PatientID': this.patientMainDicomTags['PatientID'],
                'PatientName': this.patientMainDicomTags['PatientName'],
            }
        },
        destinationContainsValidEmailAddresses() {
            if (!this.emailDestination) {
                return false;
            }
            // Regex to validate individual email addresses
            const emailRegex = /^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$/;

            // Split the string into individual email addresses
            const emails = this.emailDestination.split(/[;,\s]+/).filter(email => email);

            // Check if all entries are valid email addresses
            return emails.every(email => emailRegex.test(email));
        }
    },
    components: { CopyToClipboardButton, TextEditor }

};
</script>

<template>
    <div class="modal fade" :id="this.id" tabindex="-1" aria-labelledby="modalLabel" ref="modal-main-div">
        <!-- aria-hidden="true" -->
        <div class="modal-dialog modal-lg modal-dialog-scrollable">
            <div class="modal-content">
                <div class="modal-header">
                    <div class="d-flex flex-column me-auto">
                        <h5 class="modal-title" id="modalLabel" v-if="!isBulkSelection">{{ $t("share.modal_title") + " "
                            +
                            resourceTitle }} </h5>
                        <h5 class="modal-title" id="modalLabel" v-if="isBulkSelection">{{
                            $t("share.modal_title_multiple_studies", { count: studiesCount }) }} </h5>
                        <p v-if="uiOptions.ShareDuration > 0" class="text-muted small mb-0">{{
                            $t("share.link_expires_in", { 'count': this.uiOptions.ShareDuration}) }}</p>
                        <p v-if="uiOptions.ShareDuration == 0" class="text-muted small mb-0">{{
                            $t("share.link_never_expires") }}</p>
                    </div>
                    <button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="Close"></button>
                </div>
                <div class="modal-body">
                    <div class="container">
                        <div class="row pb-3">
                            <div class="col-md-10">
                                <div class="container py-3">
                                    <div class="row">
                                        <div class="col-md-1">
                                            {{ $t("share.link") }}
                                        </div>
                                        <div class="col-md-11">
                                            <textarea id="txt_input" v-model="shareLink" rows="8" style="min-width: 100% !important; font-family: 'Courier New', Courier, monospace;
                    font-size: 0.7em;" />
                                        </div>
                                    </div>
                                </div>
                            </div>
                            <div class="col-md-2">
                                <div class="py-3">
                                    <button v-if="shareLink != ''" type="button" class="btn btn-primary"
                                        data-bs-dismiss="modal" @click="copyAndClose()">{{ $t("share.copy_and_close")
                                        }}</button>
                                </div>
                            </div>
                        </div>
                        <div v-if="enableShareByEmail" class="row border-top pt-3">
                            <div class="col-md-10">
                                <div class="container py-3">
                                    <div class="row py-1">
                                        <div class="col-md-3">
                                            {{ $t("share.email_recipient") }}
                                        </div>
                                        <div class="col-md-9">
                                            <input type="text" class="form-control" v-model="emailDestination" />
                                        </div>
                                    </div>
                                    <div class="row py-1">
                                        <div class="col-md-3">
                                            {{ $t("share.email_title") }}
                                        </div>
                                        <div class="col-md-9">
                                            <input type="text" class="form-control" v-model="emailTitle" />
                                        </div>
                                    </div>
                                    <div class="container w-100 py-1">
                                        <TextEditor ref="editor" targetUsage="email" :modelValue="emailContent"
                                            :insertableTexts="insertableTags" />
                                    </div>
                                </div>
                            </div>
                            <div class="col-md-2">
                                <div class="py-3">
                                    <button type="button" class="btn btn-primary" data-bs-dismiss="modal"
                                        @click="sendEmail()" :disabled="!destinationContainsValidEmailAddresses">{{
                                            $t("share.send_email") }}</button>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
                <div class="modal-footer">
                    <button type="button" class="btn btn-secondary" data-bs-dismiss="modal">{{ $t("cancel")
                    }}</button>
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
</style>