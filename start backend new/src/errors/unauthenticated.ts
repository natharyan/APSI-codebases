import { HttpStatusCodes } from '../enum/StatusCode'
import { CustomAPIError } from './custom-api'

class UnauthenticatedError extends CustomAPIError {
  statusCode: any
  constructor(message: any) {
    super(message)
    this.statusCode = HttpStatusCodes.UNAUTHORIZED
  }
}

export { UnauthenticatedError }
