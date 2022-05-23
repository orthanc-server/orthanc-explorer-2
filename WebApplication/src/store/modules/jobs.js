import api from "../../orthancApi"

// job = {
//     'id': '123'
//     'name': "Send to DICOM PACS"
//     'isRunning': false,
//     'status': //response from orthanc /jobs/...
// }

///////////////////////////// STATE
const state = () => ({
    jobs: {},       // map of jobs
    jobsIds: [],    // jobs ids created by the user during this session
    maxJobsInHistory: 5
})

///////////////////////////// GETTERS
const getters = {
}

///////////////////////////// MUTATIONS

const mutations = {
    addJob(state, { jobId, name }) {
        const job = {
            'id': jobId,
            'name': name,
            'isRunning': true,
            'status': null
        }
        state.jobsIds.push(jobId);
        state.jobs[jobId] = job;
    },
    removeJob(state, { jobId }) {
        const pos = state.jobsIds.indexOf(jobId);
        if (pos >= 0) {
            state.jobsIds.splice(pos, 1);
        }
        delete state.jobs[jobId];
    }
}

///////////////////////////// ACTIONS

const actions = {
    addJob({ commit, state }, payload) {
        const jobId = payload['jobId'];
        const name = payload['name'];
        commit('addJob', { jobId: jobId, name: name });

        if (this.state.configuration.uiOptions.MaxMyJobsHistorySize > 0) {
            while (state.jobsIds.length >  this.state.configuration.uiOptions.MaxMyJobsHistorySize) {
                commit('removeJob', { jobId: state.jobsIds[0] })
            }
        }
    },
    removeJob({ commit }, payload) {
        const jobId = payload['jobId'];
        commit('removeJob', { jobId: jobId });
    },
}



///////////////////////////// EXPORT

export default {
    namespaced: true,
    state,
    getters,
    mutations,
    actions,
}