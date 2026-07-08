import api from "../../orthancApi"
import SourceType from "../../helpers/source-type";
import store from "../../store"


///////////////////////////// STATE
const state = () => ({
    selectedStudiesIds: [],
    selectedStudies: [],
})

///////////////////////////// GETTERS
const getters = {
}

///////////////////////////// MUTATIONS

const mutations = {
    selectStudy(state, { studyId, isSelected }) {
        if (isSelected && !state.selectedStudiesIds.includes(studyId)) {
            state.selectedStudiesIds.push(studyId);
            state.selectedStudies = store.state.studies.studies.filter(s => state.selectedStudiesIds.includes(s["ID"]));
        } else if (!isSelected) {
            const pos = state.selectedStudiesIds.indexOf(studyId);
            if (pos >= 0) {
                state.selectedStudiesIds.splice(pos, 1);
                state.selectedStudies = state.selectedStudies.filter(s => s["ID"] != studyId);
            }
        }
    },
    removeStudy(state, { studyId }) {
        // also delete from selection
        const pos2 = state.selectedStudiesIds.indexOf(studyId);
        if (pos2 >= 0) {
            state.selectedStudiesIds.splice(pos2, 1);
        }
    },
    selectAllStudies(state, { isSelected }) {
        if (isSelected) {
            state.selectedStudiesIds = [...store.state.studies.studiesIds];
            state.selectedStudies = [...store.state.studies.studies];
        } else {
            state.selectedStudiesIds = [];
            state.selectedStudies = [];
        }

    },
    clearSelection(state) {
        state.selectedStudiesIds = [];
        state.selectAllStudies = [];
    }
}

///////////////////////////// ACTIONS

const actions = {
    async selectStudy({ commit }, payload) {
        const studyId = payload['studyId'];
        const isSelected = payload['isSelected'];
        commit('selectStudy', { studyId: studyId, isSelected: isSelected });
    },
    async selectAllStudies({ commit }, payload) {
        const isSelected = payload['isSelected'];
        commit('selectAllStudies', { isSelected: isSelected });
    },
    async deletedStudy({ commit }, payload) {
        const studyId = payload['studyId'];
        commit('removeStudy', { studyId: studyId });
    },
    async clearSelection({ commit }) {
        commit('clearSelection');
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