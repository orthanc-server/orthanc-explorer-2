import api from "../../orthancApi"

const _clearedFilter = {
    StudyDate : "",
    AccessionNumber: "",
    PatientID: "",
    PatientName: "",
    PatientBirthDate: "",
    StudyDescription: "",
    StudyInstanceUID: "",
}

///////////////////////////// STATE
const state = () => ({
    studies: [],  // studies as returned by tools/find
    studiesIds: [],
    filters: {..._clearedFilter},
    statistics: {},
})

///////////////////////////// GETTERS
const getters = {
    filterQuery: (state, getters) => {
        let query = {};
        if (state.filters.StudyDate.length >= 8) {
            query["StudyDate"] = state.filters.StudyDate;
        }
        if (state.filters.AccessionNumber.length >= 1) {
            query["AccessionNumber"] = "*" + state.filters.AccessionNumber + "*";
        }
        if (state.filters.PatientID.length >= 1) {
            query["PatientID"] = "*" + state.filters.PatientID + "*";
        }
        if (state.filters.PatientName.length >= 1) {
            query["PatientName"] = "*" + state.filters.PatientName + "*";
        }
        if (state.filters.PatientBirthDate.length >= 8) {
            query["PatientBirthDate"] = state.filters.PatientBirthDate;
        }
        if (state.filters.StudyDescription.length >= 1) {
            query["StudyDescription"] = "*" + state.filters.StudyDescription + "*";
        }
        if (state.filters.StudyInstanceUID.length >= 1) {
            query["StudyInstanceUID"] = state.filters.StudyInstanceUID;
        }
        return query;
    }
}

///////////////////////////// MUTATIONS

const mutations = {
    setStudiesIds(state, { studiesIds }) {
        state.studiesIds = studiesIds;
    },
    setStudies(state, { studies }) {
        state.studies = studies;
    },
    addStudyId(state, { studyId }) {
        if (!state.studiesIds.includes(studyId)) {
            state.studiesIds.push(studyId);
        }
    },
    setFilter(state, { dicomTagName, value }) {
        state.filters[dicomTagName] = value;
    },
    clearFilter(state) {
        state.filters = {..._clearedFilter};
    },
    deleteStudy(state, {studyId}) {
        const pos = state.studiesIds.indexOf(studyId);
        if (pos >= 0) {
            state.studiesIds.splice(pos, 1);
        }
        state.studies = state.studies.map(s => s["ID"] != studyId);
    },
    setStatistics(state, {statistics}) {
        state.statistics = statistics;
    }
}

///////////////////////////// ACTIONS

const actions = {
    async initialLoad({ commit, state}) {
        this.dispatch('studies/loadStatistics');
    },
    async updateFilter({ commit }, payload) {
        const dicomTagName = payload['dicomTagName'];
        const value = payload['value'];
        commit('setFilter', { dicomTagName, value })

        this.dispatch('studies/reloadFilteredStudies');
    },
    async updateFilterNoReload({ commit }, payload) {
        const dicomTagName = payload['dicomTagName'];
        const value = payload['value'];
        commit('setFilter', { dicomTagName, value })
    },
    async clearFilter({ commit, state }) {
        commit('clearFilter');

        this.dispatch('studies/reloadFilteredStudies');
    },
    async clearFilterNoReload({ commit }) {
        commit('clearFilter');
    },
    async reloadFilteredStudies({ commit, getters }) {

        const studies = (await api.findStudies(getters.filterQuery)).data;
        let studiesIds = studies.map(s => s['ID']);
        commit('setStudiesIds', { studiesIds: studiesIds });
        commit('setStudies', { studies: studies });
    },
    async loadStatistics({ commit }) {
        const statistics = (await api.getStatistics()).data;
        commit('setStatistics', { statistics: statistics });
    },
    async deleteStudy({ commit }, payload) {
        const studyId = payload['studyId'];
        commit('deleteStudy', { studyId });
        this.dispatch('studies/loadStatistics');
    },
    async addStudyId({ commit }, payload) {
        const studyId = payload['studyId'];
        commit('addStudyId', { studyId });
        this.dispatch('studies/loadStatistics');
    }

}



///////////////////////////// EXPORT

export default {
    namespaced: true,
    state,
    getters,
    mutations,
    actions,
}