import api from "../../orthancApi"

///////////////////////////// STATE
const state = () => ({
    allLabels: []    // all labels in Orthanc
})

///////////////////////////// GETTERS
const getters = {
}

///////////////////////////// MUTATIONS

const mutations = {
    setLabels(state, { allLabels }) {
        state.allLabels = allLabels;
    }
}

///////////////////////////// ACTIONS

const actions = {
    async refresh({ commit }) {
        const allLabels = await api.loadAllLabels();
        commit('setLabels', { allLabels: allLabels });
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