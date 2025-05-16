import { CustomAPIError } from '../errors'

const chechPermissions = (requestUser: any, resourceUserId: any) => {
  // console.log(requestUser)
  // console.log(resourceUserId)
  // console.log(typeof resourceUserId);
  if (requestUser.role === 'admin') return
  if (requestUser.userId === resourceUserId.toString()) return
  throw new CustomAPIError.UnauthorizedError(
    'Not authorized to access this route'
  )
}

export { chechPermissions } 
