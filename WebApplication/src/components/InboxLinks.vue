<script>
import api from "../orthancApi"
import { mapState } from "vuex"
import dateHelpers from "../helpers/date-helpers"
import clipboardHelpers from "../helpers/clipboard-helpers"
import emailHelpers from "../helpers/email-helpers.js"
import TextEditor from "./TextEditor.vue";
import bootstrap from "bootstrap/dist/js/bootstrap.bundle.min.js"
import Toasts from "./Toasts.vue";


export default {
    props: [],
    data() {
        return {
            inboxLink: "",
            templateEmailContent: null,
            emailDestinationInLastGeneratedLink: "",
            emailContent: "",
            emailTitle: "",
            emailDestination: "",
            sendEmailErrorMessage: null,
            sendEmailInProgress: false
        };
    },
    computed: {
        ...mapState({
            uiOptions: state => state.configuration.uiOptions,
            isConfigurationLoaded: state => state.configuration.loaded,
        }),
        enableInboxLinksByEmail() {
            return this.uiOptions.EnableInboxLinksByEmail;
        },
        destinationContainsValidEmailAddresses() {
            return emailHelpers.isValidEmailDestination(this.emailDestination);
        },
        destinationHasChangedWrtGeneratedLink() {
            return this.emailDestination != this.emailDestinationInLastGeneratedLink;
        },
        canEditOrSendEmail() {
            return this.destinationContainsValidEmailAddresses & !this.destinationHasChangedWrtGeneratedLink;
        },
    },
    watch: {
    },
    async mounted() {
        this.reset();
    },
    methods: {
        async reset() {
            this.inboxLink = "";
            this.emailDestination = "";
            this.emailTitle = this.uiOptions.InboxLinksEmailTitle;
            this.sendEmailErrorMessage = null;
            this.sendEmailInProgress = false;
        },
        async generateLink() {
            this.emailDestinationInLastGeneratedLink = this.emailDestination;
            let token = await api.createToken({
                tokenType: 'inbox-link',  // defined in configuration file
                resources: [],
                username: this.emailDestination,
                validityDuration: this.uiOptions.InboxLinkDuration * 24 * 3600
            })
            this.inboxLink = token["Url"];
            if (this.uiOptions.EnableInboxLinksByEmail) {
                if (!this.templateEmailContent) {
                    this.templateEmailContent = await api.getEmailTemplate(this.uiOptions.InboxLinksEmailContentTemplate);
                    this.emailTitle = this.uiOptions.InboxLinksEmailTitle;
                }
                this.emailContent = this.templateEmailContent.replace('{InboxLinkUrl}', this.inboxLink);
            }
        },
        copy() {
            clipboardHelpers.copyToClipboard(this.inboxLink);
        },
        closeEmailErrorMessage() {
            this.sendEmailErrorMessage = null;
        },
        async sendEmail() {
            // console.log("Sending email to ", this.emailContent);
            this.sendEmailInProgress = true;
            let response = await api.sendEmail(this.emailDestination, this.emailTitle, this.emailContent, this.uiOptions.ShareEmailLayoutTemplate);
            if (response.success) {
                this.messageBus.emit("show-toast", this.$t('share.email_sent'));
                this.sendEmailInProgress = false;
            } else {
                this.sendEmailErrorMessage = response.details;
                this.sendEmailInProgress = false;
                console.error('Error while sending email: ', response.details)
            }
        }
    },
    components: { TextEditor, Toasts }
}
</script>

<template>
    <div class="container">
        <div class="row pb-3">
            <div class="col-md-10">
                <div class="container py-3">
                    <div class="row py-1">
                        <div class="col-md-3">
                            {{ $t("share.email_recipient") }}
                        </div>
                        <div class="col-md-9">
                            <input type="text" class="form-control" v-model="emailDestination" id="inbox-links-email"/>
                        </div>
                    </div>
                </div>
            </div>
            <div class="col-md-2">
                <div class="py-3">
                    <button v-if="inboxLink == '' || destinationHasChangedWrtGeneratedLink" type="button" class="btn btn-primary"
                        @click="generateLink()" :disabled="!destinationContainsValidEmailAddresses" id="inbox-links-generate">{{ $t("inbox_links.generate")
                        }}</button>
                </div>
            </div>
        </div>
        <div v-if="!enableInboxLinksByEmail" class="row border-top pt-3">
            <div class="row">
                <div class="col-md-10">
                    <div class="container py-3">
                        <div class="row">
                            <div class="col-md-3">
                                {{ $t("share.link") }}
                            </div>
                            <div class="col-md-9">
                                <textarea disabled id="inbox_link_txt" v-model="inboxLink" rows="8" style="min-width: 100% !important; font-family: 'Courier New', Courier, monospace;
            font-size: 0.7em;" />
                            </div>
                        </div>
                    </div>
                </div>
                <div class="col-md-2">
                    <div class="py-3">
                        <button v-if="inboxLink != ''" type="button" class="btn btn-primary" :disabled="!canEditOrSendEmail"
                            @click="copy()">{{ $t("inbox_links.copy")
                            }}</button>
                    </div>
                </div>
            </div>
        </div>
        <div v-if="enableInboxLinksByEmail" class="row border-top pt-3">
            <div v-if="sendEmailErrorMessage" class="col-md-12">
                <div class="alert alert-danger fade show d-flex" role="alert" ref="email-error-message">
                    <div class="me-auto">
                        <p>{{ $t('share.failed_to_send_email') }}</p>
                        <p>{{ sendEmailErrorMessage }}</p>
                    </div>
                    <button type="button" @click="closeEmailErrorMessage()" class="btn-close" aria-label="Close"></button>
                </div>
            </div>
            <div class="col-md-10">
                <div class="container py-3">
                    <div class="row py-1">
                        <div class="col-md-3">
                            {{ $t("share.email_title") }}
                        </div>
                        <div class="col-md-9">
                            <input type="text" class="form-control" v-model="emailTitle" :disabled="!canEditOrSendEmail"/>
                        </div>
                    </div>
                    <div class="container w-100 py-1">
                        <TextEditor ref="editor" targetUsage="email" :modelValue="emailContent"
                            :insertableTexts="{}" :disabled="!canEditOrSendEmail"/>
                    </div>
                </div>
            </div>
            <div class="col-md-2">
                <div class="py-3">
                    <button type="button" class="btn btn-primary" 
                        @click="sendEmail()" :disabled="!canEditOrSendEmail" id="inbox-links-send-email">
                    <span v-if="!sendEmailInProgress">{{ $t('share.send_email') }}</span>
                    <span v-if="sendEmailInProgress" class="spinner-border spinner-border-sm alert-icon"
                        role="status" aria-hidden="true"></span>
                    </button>
                </div>
            </div>
        </div>
    </div>
    <Toasts />
</template>

<style>
</style>