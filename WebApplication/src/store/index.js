import { createStore, createLogger } from 'vuex'
import configuration from './modules/configuration'
import studies from './modules/studies'
import jobs from './modules/jobs'

const debug = process.env.NODE_ENV !== 'production'

export default createStore({
  modules: {
    studies,
    configuration,
    jobs
  },
  strict: debug,
  plugins: debug ? [createLogger()] : []
})