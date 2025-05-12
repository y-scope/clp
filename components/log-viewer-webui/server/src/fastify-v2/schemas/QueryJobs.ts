import { Static, Type } from '@sinclair/typebox'
import { StringSchema } from './common.js'

export const QueryJobsSchema = Type.Object({
  id: StringSchema,
  status: StringSchema,
  type: StringSchema,
  job_config: StringSchema,
})

export interface QueryJobs extends Static<typeof QueryJobsSchema> {}
