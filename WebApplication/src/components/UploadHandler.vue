<script>
import Uppie from "uppie/uppie.min.js"
import UploadReport from "./UploadReport.vue"
import api from "../orthancApi"
import { compileTemplate } from "vue/compiler-sfc";

// Drop handler function to get all files
async function getAllFileEntries(dataTransferItemList) {
    let fileEntries = [];
    // Use BFS to traverse entire directory/file structure
    let queue = [];
    // Unfortunately dataTransferItemList is not iterable i.e. no forEach
    for (let i = 0; i < dataTransferItemList.length; i++) {
        queue.push(dataTransferItemList[i].webkitGetAsEntry());
    }
    while (queue.length > 0) {
        let entry = queue.shift();
        if (entry.isFile) {
            fileEntries.push(entry);
        } else if (entry.isDirectory) {
            let reader = entry.createReader();
            queue.push(...await readAllDirectoryEntries(reader));
        }
    }
    return fileEntries;
}

async function getFileFromFileEntry(fileEntry) {
    try {
        return await new Promise((resolve, reject) => fileEntry.file(resolve, reject));
    } catch (err) {
        console.log(err);
    }
}

async function getAllFiles(dataTransferItemList) {
    let fileEntries = await getAllFileEntries(dataTransferItemList);

    let files = [];
    for (let fileEntry of fileEntries) {
        files.push(await getFileFromFileEntry(fileEntry));
    }

    return files;
}

// Get all the entries (files or sub-directories) in a directory by calling readEntries until it returns empty array
async function readAllDirectoryEntries(directoryReader) {
    let entries = [];
    let readEntries = await readEntriesPromise(directoryReader);
    while (readEntries.length > 0) {
        entries.push(...readEntries);
        readEntries = await readEntriesPromise(directoryReader);
    }
    return entries;
}

// Wrap readEntries in a promise to make working with readEntries easier
async function readEntriesPromise(directoryReader) {
    try {
        return await new Promise((resolve, reject) => {
            directoryReader.readEntries(resolve, reject);
        });
    } catch (err) {
        console.log(err);
    }
}

function readFileAsync(file) {
    return new Promise((resolve, reject) => {
        let reader = new FileReader();

        reader.onload = () => {
            resolve(reader.result);
        };

        reader.onerror = reject;

        reader.readAsArrayBuffer(file);
    })
}

export default {
    props: [],
    data() {
        return {
            uppie: null,
            uploadCounter: 0,
            lastUploadReports: {}
        };
    },
    mounted() {
        this.uppie = new Uppie();
        this.uppie(document.querySelector("#filesUpload"), this.uppieUploadHandler);
        this.uppie(document.querySelector("#foldersUpload"), this.uppieUploadHandler);
    },
    methods: {
        onDrop(ev) {
            console.log("on drop", ev);
            ev.preventDefault();

            getAllFiles(ev.dataTransfer.items).then((files) => {
                this.uploadFiles(files);
            });
        },
        onDragOver(event) {
            event.preventDefault();
        },
        onDeletedUploadReport(uploadReportId) {
            delete this.lastUploadReports[uploadReportId];
        },
        async uploadedFile(uploadId, uploadedFileResponse) {
            let studyId = uploadedFileResponse["ParentStudy"];
            if (!(studyId in this.lastUploadReports[uploadId].uploadedStudiesMainDicomTags)) {
                this.lastUploadReports[uploadId].uploadedStudiesMainDicomTags[studyId] = {};
                const studyResponse = await api.getStudy(studyId);
                this.lastUploadReports[uploadId].uploadedStudiesMainDicomTags[studyId] = studyResponse.data["MainDicomTags"];

                // TODO tools/Find the study and add
                this.$store.dispatch('studies/addStudyId', {studyId: studyId});
            }
        },
        async uploadFiles(files) {
            let uploadId = this.uploadCounter++;

            this.lastUploadReports[uploadId] = {
                id: uploadId,
                filesCount: files.length,
                successFilesCount: 0,
                failedFilesCount: 0,
                skippedFilesCount: 0,
                uploadedStudiesIds: new Set(),
                uploadedStudiesMainDicomTags: {},
                errorMessages: {}
            };
            for (let file of files) {
                let filename = file.webkitRelativePath || file.name;
                if (file.name == "DICOMDIR") {
                    console.log("upload: skipping DICOMDIR file");
                    this.lastUploadReports[uploadId].skippedFilesCount++;
                    this.lastUploadReports[uploadId].errorMessages[filename] = "skipped";
                    continue;
                }
                const fileContent = await readFileAsync(file);
                try {
                    const uploadResponse = await api.uploadFile(fileContent);

                    this.lastUploadReports[uploadId].successFilesCount++;

                    if (Array.isArray(uploadResponse.data)) { // we have uploaded a zip
                        for (let uploadFileResponse of uploadResponse.data) {
                            this.uploadedFile(uploadId, uploadFileResponse);
                        }
                    } else {
                        this.uploadedFile(uploadId, uploadResponse.data);
                    }
                }
                catch (error) {
                    console.log(error);
                    let errorMessage = "error " + error.response.status;
                    if (error.response.status >= 400 && error.response.status < 500) {
                        errorMessage = error.response.data.Message;
                    }
                    this.lastUploadReports[uploadId].failedFilesCount++;
                    this.lastUploadReports[uploadId].errorMessages[filename] = errorMessage;
                }
            }
        },
        async uppieUploadHandler(event, formData, files) {
            this.uploadFiles(event.target.files);
        }
    },
    components: { UploadReport }
}
</script>

<template>
    <div>
        <div class="upload-handler-drop-zone" @drop="this.onDrop" @dragover="this.onDragOver">
            <div class="mb-3">Drop files here or</div>
                <div class="mb-3">
                <label class="btn btn-primary btn-file">
                Select folder <input type="file" style="display: none;" id="foldersUpload" required multiple directory webkitdirectory allowdirs>
                </label>
            </div>
            <div class="mb-3">
                <label class="btn btn-primary btn-file">
                Select files <input type="file" style="display: none;" id="filesUpload" required multiple>
                </label>
            </div>
        </div>
        <div class="upload-report-list">
            <UploadReport
                v-for="(upload, key) in lastUploadReports"
                :report="upload"
                :key="key"
                @deletedUploadReport="onDeletedUploadReport"
            ></UploadReport>
        </div>
    </div>
</template>
<style scoped>
.upload-report-list {
    display: flex;
    flex-direction: column-reverse;
}

.jobs {
    color: black;
}

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
}

.job-card-close {
    position: absolute;
    top: 5px;
    right: 5px;
}

.upload-details li {
    border-left: 0px;
    text-align: left;
    border: 0px;
}

.upload-handler-drop-zone {
    border-color: white;
    border-style: dashed;
    border-width: 4px;
}
</style>