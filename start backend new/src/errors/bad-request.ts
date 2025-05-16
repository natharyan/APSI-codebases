import { HttpStatusCodes } from '../enum/StatusCode'
import { CustomAPIError } from './custom-api'

class BadRequestError extends CustomAPIError {
  statusCode: any
  constructor(message: any) {
    super(message)
    this.statusCode = HttpStatusCodes.BAD_REQUEST
  }
}

export { BadRequestError }
