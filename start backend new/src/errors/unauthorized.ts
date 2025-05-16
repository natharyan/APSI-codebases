import { HttpStatusCodes } from '../enum/StatusCode'
import { CustomAPIError } from './custom-api'

class UnauthorizedError extends CustomAPIError {
  statusCode: any
  constructor(message: any) {
    super(message)
    this.statusCode = HttpStatusCodes.FORBIDDEN
  }
}

export { UnauthorizedError }
