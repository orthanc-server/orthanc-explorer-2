// store/modules/devices.js
export default {
    namespaced: true,
    
    state: {
        modalities: [],
        isLoading: false,
        isSaving: false,
        error: null,
        testResults: {}
    },
    
    mutations: {
        SET_MODALITIES(state, modalities) {
            state.modalities = modalities;
        },
        SET_LOADING(state, status) {
            state.isLoading = status;
        },
        SET_SAVING(state, status) {
            state.isSaving = status;
        },
        SET_ERROR(state, error) {
            state.error = error;
        },
        SET_TEST_RESULT(state, { id, result }) {
            state.testResults = { ...state.testResults, [id]: result };
        },
        CLEAR_TEST_RESULT(state, id) {
            const { [id]: _, ...rest } = state.testResults;
            state.testResults = rest;
        }
    },
    
    actions: {
        async loadModalities({ commit }, api) {
            commit('SET_LOADING', true);
            commit('SET_ERROR', null);
            try {
                const response = await api.getModalities();
                // Преобразуем объект в массив
                const modalities = Object.entries(response || {}).map(([id, config]) => ({
                    id,
                    ...config
                }));
                commit('SET_MODALITIES', modalities);
                return modalities;
            } catch (error) {
                commit('SET_ERROR', error.message);
                throw error;
            } finally {
                commit('SET_LOADING', false);
            }
        },
        
        async saveModality({ commit, dispatch }, { api, id, config, isNew }) {
            commit('SET_SAVING', true);
            try {
                await api.createOrUpdateModality(id, config);
                await dispatch('loadModalities', api);
            } finally {
                commit('SET_SAVING', false);
            }
        },
        
        async deleteModality({ commit, dispatch }, { api, id }) {
            try {
                await api.deleteModality(id);
                await dispatch('loadModalities', api);
            } catch (error) {
                commit('SET_ERROR', error.message);
                throw error;
            }
        },
        
        async testModality({ commit }, { api, id }) {
            commit('CLEAR_TEST_RESULT', id);
            try {
                const result = await api.testModalityEcho(id);
                commit('SET_TEST_RESULT', { 
                    id, 
                    result: { success: true, data: result } 
                });
                return result;
            } catch (error) {
                commit('SET_TEST_RESULT', { 
                    id, 
                    result: { success: false, error: error.message } 
                });
                throw error;
            }
        }
    },
    
    getters: {
        getModalityById: (state) => (id) => {
            return state.modalities.find(m => m.id === id);
        },
        hasModalities: (state) => state.modalities.length > 0
    }
};